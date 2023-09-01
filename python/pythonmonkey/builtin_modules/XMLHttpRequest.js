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
const { request, decodeStr } = require('XMLHttpRequest-internal');

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
/**
 * Implement the `XMLHttpRequest` API (`XHR` for short) according to the spec.
 * @see https://xhr.spec.whatwg.org/
 */
class XMLHttpRequest extends XMLHttpRequestEventTarget
{
  // event handler
  /** @type {EventListenerFn} */
  onreadystatechange = null;

  // 
  // states
  // 
  /** @readonly */ static UNSENT           = 0;
  /** @readonly */ static OPENED           = 1;
  /** @readonly */ static HEADERS_RECEIVED = 2;
  /** @readonly */ static LOADING          = 3;
  /** @readonly */ static DONE             = 4;

  /** @readonly */ UNSENT           = 0;
  /** @readonly */ OPENED           = 1;
  /** @readonly */ HEADERS_RECEIVED = 2;
  /** @readonly */ LOADING          = 3;
  /** @readonly */ DONE             = 4;

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
    this.#receivedBytes = [];
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
   * Timeout time in **milliseconds**.  
   * When set to a non-zero value will cause fetching to terminate after the given time has passed.
   */
  timeout = 0;

  get withCredentials()
  {
    return false;
  }
  set withCredentials(flag)
  {
    throw new DOMException('xhr.withCredentials is not supported in PythonMonkey.', 'InvalidAccessError');
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

    // FIXME: do we have to initiate request here instead of in #sendAsync? (step 6)

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
    let responseLength = 0;
    const processResponse = (response) =>
    {
      this.#response = response; // step 11.9.1
      this.#state = XMLHttpRequest.HEADERS_RECEIVED; // step 11.9.4
      this.dispatchEvent(new Event('readystatechange')); // step 11.9.5
      if (this.#state !== XMLHttpRequest.HEADERS_RECEIVED) // step 11.9.6
        return;
      responseLength = this.#response.contentLength; // step 11.9.8
    };

    const processBodyChunk = (/** @type {Uint8Array} */ bytes) =>
    {
      this.#receivedBytes.push(bytes);
      if (this.#state === XMLHttpRequest.HEADERS_RECEIVED)
        this.#state = XMLHttpRequest.LOADING;
      this.dispatchEvent(new Event('readystatechange'));
      this.dispatchEvent(new ProgressEvent('progress', { loaded:this.#receivedLength, total:responseLength }));
    };

    /**
     * @see https://xhr.spec.whatwg.org/#handle-response-end-of-body
     */
    const processEndOfBody = () =>
    {
      const transmitted = this.#receivedLength; // step 3
      const length = responseLength || 0; // step 4
      this.dispatchEvent(new ProgressEvent('progress', { loaded:transmitted, total:length })); // step 6
      this.#state = XMLHttpRequest.DONE; // step 7
      this.#sendFlag = false; // step 8
      this.dispatchEvent(new Event('readystatechange')); // step 9
      for (const eventType of ['load', 'loadend']) // step 10, step 11
        this.dispatchEvent(new ProgressEvent(eventType, { loaded:transmitted, total:length }));
    };

    // send() step 6
    request(
      this.#requestMethod,
      this.#requestURL.toString(),
      this.#requestHeaders,
      this.#requestBody ?? '',
      this.timeout,
      processRequestBodyChunkLength,
      processRequestEndOfBody,
      processResponse,
      processBodyChunk,
      processEndOfBody,
      () => (this.#timedOutFlag = true), // onTimeoutError
      () => (this.#response = null /* network error */), // onNetworkError
    ).catch((e) => this.#handleErrors(e));
  }

  /**
   * @see https://xhr.spec.whatwg.org/#dom-xmlhttprequest-send step 12
   */
  #sendSync()
  {
    throw new DOMException('synchronous XHR is not supported', 'NotSupportedError');
    // TODO: handle synchronous request
  }

  /**
   * @see https://xhr.spec.whatwg.org/#handle-errors
   * @param {Error} e 
   */
  #handleErrors(e)
  {
    if (!this.#sendFlag) // step 1
      return;
    if (this.#timedOutFlag) // step 2
      return this.#reportRequestError('timeout', new DOMException(e.toString(), 'TimeoutError'));
    console.error(e); // similar to browsers, print out network errors even then the error will be handled by `xhr.onerror`
    if (this.#response === null /* network error */) // step 4
      return this.#reportRequestError('error', new DOMException(e.toString(), 'NetworkError'));
    else // unknown errors
      return this.#reportRequestError('error', new DOMException(e.toString(), 'InvalidStateError'));
  }

  /**
   * @see https://xhr.spec.whatwg.org/#request-error-steps
   * @param {string} event event type
   * @param {DOMException} exception
   */
  #reportRequestError(event, exception)
  {
    this.#state = XMLHttpRequest.DONE; // step 1
    this.#sendFlag = false; // step 2

    this.#response = null/* network error */; // step 3

    if (this.#synchronousFlag) // step 4
      throw exception;

    this.dispatchEvent(new Event('readystatechange')); // step 5

    if (!this.#uploadCompleteFlag) // step 6
    {
      this.#uploadCompleteFlag = true;
      if (this.#uploadListenerFlag)
      {
        this.#uploadObject.dispatchEvent(new ProgressEvent(event, { loaded:0, total:0 }));
        this.#uploadObject.dispatchEvent(new ProgressEvent('loadend', { loaded:0, total:0 }));
      }
    }

    this.dispatchEvent(new ProgressEvent(event, { loaded:0, total:0 })); // step 7
    this.dispatchEvent(new ProgressEvent('loadend', { loaded:0, total:0 })); // step 8
  }

  /**
   * Cancels any network activity. 
   * @see https://xhr.spec.whatwg.org/#the-abort()-method
   */
  abort()
  {
    if (this.#response)
      this.#response.abort(); // step 1

    if (
      (this.#state === XMLHttpRequest.OPENED && this.#sendFlag)
      || this.#state === XMLHttpRequest.HEADERS_RECEIVED
      || this.#state === XMLHttpRequest.LOADING
    ) // step 2
      return this.#reportRequestError('abort', new DOMException('Aborted.', 'AbortError'));

    if (this.#state === XMLHttpRequest.DONE) // step 3
    {
      this.#state = XMLHttpRequest.UNSENT;
      this.#response = null; /* network error */
    }
  }

  // 
  // response
  // 
  /**
   * @return {string}
   */
  get responseURL()
  {
    if (!this.#response)
      return '';
    else
      return this.#response.url;
  }

  /**
   * @return {number} HTTP status code
   */
  get status()
  {
    if (!this.#response)
      return 0;
    else
      return this.#response.status;
  }

  /**
   * @return {string} HTTP status message
   */
  get statusText()
  {
    if (!this.#response)
      return '';
    else
      return this.#response.statusText;
  }

  /**
   * @param {string} name 
   * @return {string} the text of a particular header's value
   */
  getResponseHeader(name)
  {
    if (!this.#response)
      return null;
    else
      return this.#response.getResponseHeader(name) ?? null;
  }

  /**
   * @return {string} all the response headers, separated by CRLF, as a string, or returns null if no response has been received. 
   */
  getAllResponseHeaders()
  {
    if (!this.#response)
      return '';
    else
      return this.#response.getAllResponseHeaders();
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
    if (!['', 'text', 'arraybuffer'].includes(t))
      throw new DOMException('only responseType "text" or "arraybuffer" is supported', 'NotSupportedError');
    this.#responseType = t;
  }

  /**
   * @see https://xhr.spec.whatwg.org/#text-response
   * @return {string}
   */
  #getTextResponse()
  {
    // TODO: refactor using proper TextDecoder API
    // TODO: handle encodings other than utf-8
    this.#responseObject = decodeStr(this.#mergeReceivedBytes(), 'utf-8');
    return this.#responseObject;
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

    if (this.#responseObject) // step 4
      return this.#responseObject;
    if (this.#responseType === 'arraybuffer') // step 5
    {
      this.#responseObject = this.#mergeReceivedBytes().buffer;
      return this.#responseObject;
    }

    throw new DOMException(`unsupported responseType "${this.#responseType}"`, 'InvalidStateError');
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
  /** @type {Method} */
  #requestMethod = null;
  /** @type {URL} */
  #requestURL = null;
  /** @type {{ [name: string]: string; }} */
  #requestHeaders = {};
  /** @type {string | Uint8Array | null} */
  #requestBody = null;
  #synchronousFlag = false; // A flag, initially unset.
  #uploadCompleteFlag = false; // A flag, initially unset.
  #uploadListenerFlag = false; // A flag, initially unset.
  #timedOutFlag = false; // A flag, initially unset.
  /** @type {import('./XMLHttpRequest-internal').XHRResponse} */
  #response = null;
  /** @type {Uint8Array[]} */
  #receivedBytes = [];
  /** @type {ResponseType} */
  #responseType = '';
  /** 
   * cache for converting receivedBytes to the desired response type
   * @type {ArrayBuffer | string}
   */
  #responseObject = null;

  /**
   * Get received bytes’s total length
   */
  get #receivedLength()
  {
    return this.#receivedBytes.reduce((sum, chunk) => sum + chunk.length, 0);
  }

  /**
   * Concatenate received bytes into one single Uint8Array
   */
  #mergeReceivedBytes()
  {
    let offset = 0;
    const merged = new Uint8Array(this.#receivedLength);
    for (const chunk of this.#receivedBytes)
    {
      merged.set(chunk, offset);
      offset += chunk.length;
    }
    return merged;
  }
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
