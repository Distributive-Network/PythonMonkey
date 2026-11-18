/**
 * @file     WebSocket.js
 *           Implement the WebSocket API, backed by Python's aiohttp
 *           WebSocket client (WebSocket-internal.py).
 * @author   Dan Desjardins <dan@distributive.network>
 * @date     September 2026
 *
 * @copyright Copyright (c) 2026 Distributive Corp.
 */
'use strict';

const { EventTarget, Event } = require('event-target');
const { DOMException } = require('dom-exception');
const { URL } = require('url');
const { wsConnect } = require('WebSocket-internal');
const debug = globalThis.python.eval('__import__("pythonmonkey").bootstrap.require')('debug');

// exposed
class MessageEvent extends Event
{
  constructor(type, eventInitDict = {})
  {
    super(type);
    this.data = eventInitDict.data;
  }
}

// exposed
class CloseEvent extends Event
{
  constructor(type, eventInitDict = {})
  {
    super(type);
    this.code = eventInitDict.code ?? 1000;
    this.reason = eventInitDict.reason ?? '';
    this.wasClean = eventInitDict.wasClean ?? true;
  }
}

/**
 * Implement the `WebSocket` API according to the spec, backed by aiohttp.
 * @see https://websockets.spec.whatwg.org/
 */
class WebSocket extends EventTarget
{
  /** @readonly */ static CONNECTING = 0;
  /** @readonly */ static OPEN       = 1;
  /** @readonly */ static CLOSING    = 2;
  /** @readonly */ static CLOSED     = 3;

  /** @readonly */ CONNECTING = 0;
  /** @readonly */ OPEN       = 1;
  /** @readonly */ CLOSING    = 2;
  /** @readonly */ CLOSED     = 3;

  // event handlers -- EventTarget#dispatchEvent auto-invokes these
  onopen = null;
  onmessage = null;
  onerror = null;
  onclose = null;

  #readyState = WebSocket.CONNECTING;
  #conn = null;
  #url;
  #protocol = '';

  // engine.io-client's WebSocket transport calls `this.ws._socket.unref()` on
  // open when its autoUnref option is set, a Node `ws`-library detail that a
  // browser-style WebSocket doesn't have. Nothing here needs unref'ing.
  _socket = { unref() {}, ref() {} };

  /**
   * @param {string | URL} url
   * @param {string | string[]} [protocols]
   */
  constructor(url, protocols)
  {
    super();
    const parsedURL = new URL(url);
    if (!['ws:', 'wss:'].includes(parsedURL.protocol))
      throw new DOMException(`Invalid WebSocket URL scheme "${parsedURL.protocol}"`, 'SyntaxError');
    this.#url = parsedURL.href;

    const protoArray = protocols ? (Array.isArray(protocols) ? protocols : [protocols]) : [];

    // aiohttp's ws_connect() expects a plain http(s):// URL, not ws(s)://
    const httpURL = this.#url.replace(/^ws/, 'http');

    debug('ws:connect')(`connecting to ${httpURL}`);

    wsConnect(
      httpURL,
      protoArray,
      (protocol, sendText, sendBinary, closeFn) => // onOpen
      {
        this.#conn = { sendText, sendBinary, close: closeFn };
        if (this.#readyState === WebSocket.CLOSING) // close() was called while connecting
        {
          closeFn(1000, '');
          return;
        }
        this.#protocol = protocol;
        this.#readyState = WebSocket.OPEN;
        debug('ws:open')(`connected to ${this.#url}`);
        this.dispatchEvent(new Event('open'));
      },
      (data, isBinary) => // onMessage
      {
        // copy binary payloads out of the Python bytearray into a JS-owned ArrayBuffer
        const payload = isBinary ? new Uint8Array(data).buffer : data;
        this.dispatchEvent(new MessageEvent('message', { data: payload }));
      },
      (message) => // onError
      {
        debug('ws:error')(message);
        this.dispatchEvent(new Event('error'));
      },
      (code, reason) => // onClose
      {
        this.#readyState = WebSocket.CLOSED;
        debug('ws:close')(`closed, code=${code} reason=${reason}`);
        // 1006 is reserved for connections that dropped without a close handshake
        this.dispatchEvent(new CloseEvent('close', { code, reason, wasClean: code !== 1006 }));
      },
      debug,
    ).catch((e) => // only reachable if a callback above threw past the Python side
    {
      debug('ws:error')(String(e));
      if (this.#readyState === WebSocket.CLOSED)
        return;
      this.#readyState = WebSocket.CLOSED;
      this.dispatchEvent(new Event('error'));
      this.dispatchEvent(new CloseEvent('close', { code: 1006, reason: String(e), wasClean: false }));
    });
  }

  get readyState() { return this.#readyState; }
  get url() { return this.#url; }
  get protocol() { return this.#protocol; }
  get bufferedAmount() { return 0; } // not tracked

  /**
   * @param {string | ArrayBuffer | ArrayBufferView} data
   */
  send(data)
  {
    if (this.#readyState === WebSocket.CONNECTING)
      throw new DOMException('WebSocket is still connecting (readyState CONNECTING)', 'InvalidStateError');
    if (this.#readyState !== WebSocket.OPEN)
      return; // per spec: silently discard if not OPEN
    if (typeof data === 'string')
      this.#conn.sendText(data);
    else if (data instanceof ArrayBuffer)
      this.#conn.sendBinary(new Uint8Array(data));
    else if (ArrayBuffer.isView(data))
      this.#conn.sendBinary(new Uint8Array(data.buffer, data.byteOffset, data.byteLength));
    else
      throw new TypeError('WebSocket.send() data must be a string, ArrayBuffer or ArrayBufferView');
  }

  /**
   * @param {number} [code]
   * @param {string} [reason]
   */
  close(code = 1000, reason = '')
  {
    if (this.#readyState === WebSocket.CLOSING || this.#readyState === WebSocket.CLOSED)
      return;
    this.#readyState = WebSocket.CLOSING;
    if (this.#conn) // otherwise still connecting; onOpen closes it
      this.#conn.close(code, reason);
  }
}

/* A side-effect of loading this module is to add WebSocket and related symbols to the global
 * object, matching XMLHttpRequest.js, so code written for browsers works without a require().
 */
if (!globalThis.WebSocket)
  globalThis.WebSocket = WebSocket;
if (!globalThis.MessageEvent)
  globalThis.MessageEvent = MessageEvent;
if (!globalThis.CloseEvent)
  globalThis.CloseEvent = CloseEvent;

exports.WebSocket = WebSocket;
exports.MessageEvent = MessageEvent;
exports.CloseEvent = CloseEvent;
