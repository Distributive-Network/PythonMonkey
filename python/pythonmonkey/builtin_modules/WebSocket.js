/**
 * @file     WebSocket.js
 *           Implement the WebSocket API, backed by Python's aiohttp
 *           WebSocket client (WebSocket-internal.py).
 *
 *           LOCAL PATCH (this session): pythonmonkey has never had a
 *           WebSocket implementation, which is why dcp-client's
 *           SocketIOTransport.buildOptions() hard-codes
 *           `transports: platform !== 'pythonmonkey' ? ['websocket','polling'] : ['polling']`
 *           -- every other platform upgrades to a persistent WebSocket
 *           almost immediately, while pythonmonkey is forced to sustain
 *           long-polling for the entire connection lifetime. That
 *           sustained-polling code path is essentially unexercised by any
 *           other real client, and packages.distributed.computer's own
 *           backend has a real, reproducible session-routing bug in it
 *           (a freshly-issued session id 404s on the very next polling
 *           request, confirmed with plain curl/aiohttp/Node https/a bare
 *           engine.io-client, no dcp-client or pythonmonkey involved).
 *           Giving pythonmonkey a real WebSocket lets it take the exact
 *           same well-exercised upgrade path every other platform takes,
 *           sidestepping that bug entirely instead of working around it.
 *
 * @date     September 2026
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
  #sendBuffer = []; // messages queued before the underlying connection is ready

  // engine.io-client's WS transport (addEventListeners()) unconditionally
  // does `this.ws._socket.unref()` when its `autoUnref` option is set --
  // real Node `ws` library sockets expose the underlying raw net.Socket as
  // `._socket`, but a browser-style WebSocket has no such concept. dcp-client
  // sets `autoUnref: true` for pythonmonkey (it's in env.js's
  // referencedTimerPlatformList), so without this dummy property that call
  // throws "can't access property unref, this.ws._socket is undefined" the
  // instant the connection opens. A no-op unref() is exactly correct here:
  // there is nothing OS-level for pythonmonkey to unref in the first place.
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
      {},
      (sendText, sendBinary, closeFn) => // onOpen
      {
        // Wired up in the SAME synchronous callback that fires 'open' --
        // see WebSocket-internal.py's docstring for why this matters (a
        // message sent in reaction to 'open', which real clients commonly
        // do, must never race ahead of these being available).
        this.#conn = { sendText, sendBinary, close: closeFn };
        this.#readyState = WebSocket.OPEN;
        debug('ws:open')(`connected to ${this.#url}`);
        for (const queued of this.#sendBuffer)
          this.#doSend(queued);
        this.#sendBuffer = [];
        this.dispatchEvent(new Event('open'));
      },
      (data, isBinary) => // onMessage
      {
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
        this.dispatchEvent(new CloseEvent('close', { code, reason, wasClean: code === 1000 }));
      },
      debug,
    ).catch((e) =>
    {
      this.#readyState = WebSocket.CLOSED;
      debug('ws:error')(String(e));
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
    this.#doSend(data);
  }

  #doSend(data)
  {
    if (!this.#conn)
      return;
    if (typeof data === 'string')
      this.#conn.sendText(data);
    else if (data instanceof ArrayBuffer)
      this.#conn.sendBinary(new Uint8Array(data));
    else
      this.#conn.sendBinary(data); // TypedArray/DataView
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
    if (this.#conn)
      this.#conn.close(code, reason);
  }
}

/* A side-effect of loading this module is to add WebSocket and related
 * symbols to the global object, matching XMLHttpRequest.js's convention,
 * so real code (like dcp-client, once its platform check is relaxed) can
 * use `new WebSocket(...)` directly with no require() needed.
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
