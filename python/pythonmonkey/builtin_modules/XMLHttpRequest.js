/**
 * @file     XMLHttpRequest.js
 *           Implement the XMLHttpRequest (XHR) API
 *
 * @author   Tom Tang <xmader@distributive.network>
 * @date     August 2023
 */

const { EventTarget, Event } = require('event-target');
const { DOMException } = require('dom-exception');
const { URL, URLSearchParams } = require('url');

// exposed
/**
 * Events using the ProgressEvent interface indicate some kind of progression. 
 */
class ProgressEvent extends Event
{
  /**
   * @param {string} type
   * @param {{ lengthComputable?: boolean; loaded?: number; total?: number; }} eventInitDict
   */
  constructor (type, eventInitDict = {})
  {
    super(type);
    this.lengthComputable = eventInitDict.lengthComputable ?? false;
    this.loaded = eventInitDict.loaded ?? 0;
    this.total = eventInitDict.total ?? 0;
  }
}

// exposed
class XMLHttpRequestEventTarget extends EventTarget
{
  // event handlers
  /** @typedef {import('event-target').EventListenerFn} EventListenerFn */
  /** @type {EventListenerFn} */
  onloadstart;
  /** @type {EventListenerFn} */
  onprogress;
  /** @type {EventListenerFn} */
  onabort;
  /** @type {EventListenerFn} */
  onerror;
  /** @type {EventListenerFn} */
  onload;
  /** @type {EventListenerFn} */
  ontimeout;
  /** @type {EventListenerFn} */
  onloadend;
}

// exposed
class XMLHttpRequestUpload extends XMLHttpRequestEventTarget
{}

const FORBIDDEN_REQUEST_METHODS = [
  'TRACE',
  'TRACK',
  'CONNECT'
];

// exposed
class XMLHttpRequest extends XMLHttpRequestEventTarget
{
  // event handler
  /** @type {EventListenerFn} */
  onreadystatechange = null;

  // 
  // states
  // 
  static UNSENT = 0;
  static OPENED = 1;
  static HEADERS_RECEIVED = 2;
  static LOADING = 3;
  static DONE = 4;
  /**
   * Returns client's state.
   */
  get readyState()
  {
    return this.#state;
  }

  // 
  // request
  // 
  /**
   * Sets the request method, request URL, and synchronous flag.
   * @typedef {'DELETE' | 'GET' | 'HEAD' | 'OPTIONS' | 'POST' | 'PUT'} Method
   * @param {Method} method 
   * @param {string | URL} url 
   * @param {boolean} async 
   * @param {string} username 
   * @param {string} password 
   * @see https://xhr.spec.whatwg.org/#the-open()-method
   */
  open(method, url, async = true, username = null, password = null)
  {
    // Reset
    this.abort();

    // Normalize the method.
    // @ts-expect-error
    method = method.toString().toUpperCase();

    // Check for valid request method
    if (!method || FORBIDDEN_REQUEST_METHODS.includes(method))
      throw new DOMException('Request method not allowed', 'SecurityError');

    const parsedURL = new URL(url);
    if (username)
      parsedURL.username = username;
    if (password)
      parsedURL.password = password;
    
    // step 11
    this.#sendFlag = false;
    this.#uploadListenerFlag = false;
    this.#requestMethod = method;
    this.#requestURL = parsedURL;
    if (async === false)
      this.#synchronousFlag = true;
    this.#requestHeaders = {}; // clear
    this.#response = null;
    this.#responseObject = null;

    // step 12
    if (this.#state !== XMLHttpRequest.OPENED)
    {
      this.#state = XMLHttpRequest.OPENED;
      this.dispatchEvent(new Event('readystatechange'));
    }
  }

  /**
   * Combines a header in author request headers.
   * @param {string} name 
   * @param {string} value 
   */
  setRequestHeader(name, value)
  {
    if (this.#state !== XMLHttpRequest.OPENED)
      throw new DOMException('setRequestHeader can only be called when state is OPEN', 'InvalidStateError');
    if (this.#sendFlag)
      throw new DOMException('send flag is true', 'InvalidStateError');

    // Normalize value
    value = value.toString().trim();
    name = name.toString().trim().toLowerCase();

    // TODO: do we need to throw for forbidden request-headers here?
    // see https://fetch.spec.whatwg.org/#forbidden-request-header

    // Combine header values
    if (this.#requestHeaders[name])
      this.#requestHeaders[name] += ', ' + value;
    else
      this.#requestHeaders[name] = value;
  }

  /**
   * Returns the associated XMLHttpRequestUpload object.  
   * It can be used to gather transmission information when data is transferred to a server. 
   */
  get upload()
  {
    return this.#uploadObject;
  }

  /**
   * Initiates the request.
   * @typedef {TypedArray | DataView | ArrayBuffer | URLSearchParams | string} XMLHttpRequestBodyInit
   * @param {XMLHttpRequestBodyInit | null} body 
   * @see https://xhr.spec.whatwg.org/#dom-xmlhttprequest-send
   */
  send(body = null)
  {
    if (this.#state !== XMLHttpRequest.OPENED) // step 1
      throw new DOMException('connection must be opened before send() is called', 'InvalidStateError');
    if (this.#sendFlag) // step 2
      throw new DOMException('send has already been called', 'InvalidStateError');

    if (['GET', 'HEAD'].includes(this.#requestMethod)) // step 3
      body = null;

    // step 4
    this.#requestBody = null;
    if (body !== null)
    {
      let extractedContentType = null;
      if (body instanceof URLSearchParams)
      {
        this.#requestBody = body.toString();
        extractedContentType = 'application/x-www-form-urlencoded;charset=UTF-8';
      }
      else if (typeof body === 'string')
      {
        this.#requestBody = body;
        extractedContentType = 'text/plain;charset=UTF-8';
      }
      else // BufferSource
      {
        this.#requestBody = body instanceof ArrayBuffer ? new Uint8Array(body) : new Uint8Array(body.buffer); // make a copy
      }

      const originalAuthorContentType = this.#requestHeaders['content-type'];
      if (!originalAuthorContentType && extractedContentType)
      {
        this.#requestHeaders['content-type'] = extractedContentType;
      }
    }

    // step 5
    if (this.#uploadObject._hasAnyListeners())
      this.#uploadListenerFlag = true;

    // step 6
    // TODO: Let req be a new request, initialized as follows: 

    this.#uploadCompleteFlag = false; // step 7
    this.#timedOutFlag = false; // step 8
    if (this.#requestBody === null) // step 9
      this.#uploadCompleteFlag = true;
    this.#sendFlag = true; // step 10

    if (!this.#synchronousFlag) // step 11
      this.#sendAsync();
    else // step 12
      this.#sendSync();
  }

  /**
   * @see https://xhr.spec.whatwg.org/#dom-xmlhttprequest-send step 11
   */
  #sendAsync()
  {
    this.dispatchEvent(new ProgressEvent('loadstart', { loaded:0, total:0 })); // step 11.1
    
    let requestBodyTransmitted = 0; // step 11.2
    let requestBodyLength = this.#requestBody ? this.#requestBody.length : 0; // step 11.3
    if (!this.#uploadCompleteFlag && this.#uploadListenerFlag) // step 11.5
      this.#uploadObject.dispatchEvent(new ProgressEvent('loadstart', { loaded:requestBodyTransmitted, total:requestBodyLength }));
    
    if (this.#state !== XMLHttpRequest.OPENED || !this.#sendFlag) // step 11.6
      return;
    
    // step 11.7
    const processRequestBodyChunkLength = (/** @type {number} */ bytesLength) => 
    {
      requestBodyTransmitted += bytesLength;
      if (this.#uploadListenerFlag)
        this.#uploadObject.dispatchEvent(new ProgressEvent('progress', { loaded:requestBodyTransmitted, total:requestBodyLength }));
    };

    // step 11.8
    const processRequestEndOfBody = () =>
    {
      this.#uploadCompleteFlag = true;
      if (!this.#uploadListenerFlag)
        return;
      for (const eventType of ['progress', 'load', 'loadend'])
        this.#uploadObject.dispatchEvent(new ProgressEvent(eventType, { loaded:requestBodyTransmitted, total:requestBodyLength }));
    };

    // step 11.9
    const processResponse = (response) =>
    {
      this.#response = response; // step 11.9.1
      this.#state = XMLHttpRequest.HEADERS_RECEIVED; // step 11.9.4
      this.dispatchEvent(new Event('readystatechange')); // step 11.9.5
      if (this.#state !== XMLHttpRequest.HEADERS_RECEIVED) // step 11.9.6
        return;
      // TODO
    };
    // TODO:
  }

  /**
   * @see https://xhr.spec.whatwg.org/#dom-xmlhttprequest-send step 12
   */
  #sendSync()
  {
    throw new DOMException('synchronous XHR is not supported', 'NotSupportedError');
    // TODO: handle synchronous request
  }

  abort()
  {
    // TODO
  }

  // 
  // response
  // 
  /**
   * @return {string}
   */
  get responseURL()
  {
    // TODO
  }

  /**
   * @return {number} HTTP status code
   */
  get status()
  {
    // TODO
  }

  /**
   * @return {string} HTTP status message
   */
  get statusText()
  {
    // TODO
  }

  /**
   * @param {string} name 
   * @return {string} the text of a particular header's value
   */
  getResponseHeader(name)
  {
    // TODO
  }

  /**
   * @return {string} all the response headers, separated by CRLF, as a string, or returns null if no response has been received. 
   */
  getAllResponseHeaders()
  {
    // TODO
  }

  /**
   * Acts as if the `Content-Type` header value for a response is mime.  
   * (It does not change the header.) 
   * @param {string} mime 
   */
  overrideMimeType(mime)
  {
    // TODO
  }

  /**
   * @typedef {"" | "arraybuffer" | "blob" | "document" | "json" | "text"} ResponseType
   */
  get responseType()
  {
    return this.#responseType;
  }
  set responseType(t)
  {
    if (this.#state === XMLHttpRequest.LOADING || this.#state === XMLHttpRequest.DONE)
      throw new DOMException('responseType can only be set before send()', 'InvalidStateError');
    if (!['', 'text', 'arraybuffer', 'json'].includes(t))
      throw new DOMException('only responseType "text", "arraybuffer", or "json" is supported', 'NotSupportedError');
    this.#responseType = t;
  }

  /**
   * @see https://xhr.spec.whatwg.org/#text-response
   * @return {string}
   */
  #getTextResponse()
  {
    // TODO
  }

  /**
   * Returns the response body.
   * @see https://xhr.spec.whatwg.org/#the-response-attribute
   */
  get response()
  {
    if (this.#responseType === '' || this.#responseType === 'text') // step 1
      return this.responseText;
    if (this.#state !== XMLHttpRequest.DONE) // step 2
      return null;
    // TODO
  }

  /**
   * Returns response as text.
   */
  get responseText()
  {
    if (!['text', ''].includes(this.#responseType))
      throw new DOMException('responseType must be "text" or an empty string', 'InvalidStateError');
    if (![XMLHttpRequest.LOADING, XMLHttpRequest.DONE].includes(this.#state))
      return '';
    else
      return this.#getTextResponse();
  }

  /**
   * Returns the response as document. 
   */
  get responseXML()
  {
    throw new DOMException('responseXML is not supported', 'NotSupportedError');
  }

  // internal properties
  #uploadObject = new XMLHttpRequestUpload();
  #state = XMLHttpRequest.UNSENT; // One of unsent, opened, headers received, loading, and done; initially unsent.
  #sendFlag = false; // A flag, initially unset.
  #timeout = 0; // An unsigned integer, initially 0.
  /** @type {Method} */
  #requestMethod = null;
  /** @type {URL} */
  #requestURL = null;
  #requestHeaders = {};
  /** @type {string | Uint8Array | null} */
  #requestBody = null;
  #synchronousFlag = false; // A flag, initially unset.
  #uploadCompleteFlag = false; // A flag, initially unset.
  #uploadListenerFlag = false; // A flag, initially unset.
  #timedOutFlag = false; // A flag, initially unset.
  #response = null;
  /** @type {ResponseType} */
  #responseType = '';
  #responseObject = null;
}

if (!globalThis.XMLHttpRequestEventTarget)
  globalThis.XMLHttpRequestEventTarget = XMLHttpRequestEventTarget;
if (!globalThis.XMLHttpRequestUpload)
  globalThis.XMLHttpRequestUpload = XMLHttpRequestUpload;
if (!globalThis.XMLHttpRequest)
  globalThis.XMLHttpRequest = XMLHttpRequest;
if (!globalThis.ProgressEvent)
  globalThis.ProgressEvent = ProgressEvent;

exports.XMLHttpRequestEventTarget = XMLHttpRequestEventTarget;
exports.XMLHttpRequestUpload = XMLHttpRequestUpload;
exports.XMLHttpRequest = XMLHttpRequest;
exports.ProgressEvent = ProgressEvent;
