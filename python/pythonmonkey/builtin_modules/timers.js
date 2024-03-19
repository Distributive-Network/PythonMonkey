/**
 * @file     timers.js
 * @author   Tom Tang <xmader@distributive.network>
 * @date     May 2023
 */
const internalBinding = require('internal-binding');

const { 
  enqueueWithDelay,
  cancelByTimeoutId,
  timerHasRef,
  timerAddRef,
  timerRemoveRef,
} = internalBinding('timers');

/**
 * Implement Node.js-style `timeoutId` class returned from setTimeout() and setInterval()
 * @see https://nodejs.org/api/timers.html#class-timeout
 */
class Timeout
{
  /** @type {number} an integer */
  #numericId;

  /**
   * @param {number} numericId 
   */
  constructor(numericId)
  {
    this.#numericId = numericId;
  }

  /**
   * If `true`, the `Timeout` object will keep the event-loop active.
   * @returns {boolean}
   */
  hasRef()
  {
    return timerHasRef(this.#numericId);
  }

  /**
   * When called, requests that the event-loop **not exit** so long as the `Timeout` is active.
   *   
   * By default, all `Timeout` objects are "ref'ed", making it normally unnecessary to call `timeout.ref()` unless `timeout.unref()` had been called previously.
   */
  ref()
  {
    timerAddRef(this.#numericId);
    return this; // allow chaining
  }

  /**
   * When called, the active `Timeout` object will not require the event-loop to remain active.  
   * If there is no other activity keeping the event-loop running, the process may exit before the `Timeout` object's callback is invoked.
   */
  unref()
  {
    timerRemoveRef(this.#numericId);
    return this; // allow chaining
  }

  /**
   * @returns a number that can be used to reference this timeout
   */
  [Symbol.toPrimitive]()
  {
    return this.#numericId;
  }
}

/**
 * Implement the `setTimeout` global function
 * @see https://developer.mozilla.org/en-US/docs/Web/API/setTimeout and
 * @see https://html.spec.whatwg.org/multipage/timers-and-user-prompts.html#dom-settimeout
 * @param {Function | string} handler
 * @param {number} delayMs timeout milliseconds, use value of 0 if this is omitted
 * @param {any[]} args additional arguments to be passed to the `handler`
 * @return {Timeout} timeoutId
 */
function setTimeout(handler, delayMs = 0, ...args) 
{
  // Ensure the first parameter is a function
  // We support passing a `code` string to `setTimeout` as the callback function
  if (typeof handler !== 'function')
    handler = new Function(handler);

  // `setTimeout` allows passing additional arguments to the callback, as spec-ed
  // FIXME (Tom Tang): the spec doesn't allow additional arguments to be passed if the original `handler` is not a function
  const thisArg = globalThis; // HTML spec requires `thisArg` to be the global object
  // Wrap the job function into a bound function with the given additional arguments
  //    https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Function/bind
  /** @type {Function} */
  const boundHandler = handler.bind(thisArg, ...args);

  // Get the delay time in seconds
  //  JS `setTimeout` takes milliseconds, but Python takes seconds
  delayMs = Number(delayMs) || 0; // ensure the `delayMs` is a `number`, explicitly do type coercion
  if (delayMs < 0)
    delayMs = 0; // as spec-ed
  const delaySeconds = delayMs / 1000; // convert ms to s

  return new Timeout(enqueueWithDelay(boundHandler, delaySeconds));
}

/**
 * Implement the `clearTimeout` global function
 * @see https://developer.mozilla.org/en-US/docs/Web/API/clearTimeout and
 * @see https://html.spec.whatwg.org/multipage/timers-and-user-prompts.html#dom-cleartimeout
 * @param {Timeout | number} timeoutId
 * @return {void}
 */
function clearTimeout(timeoutId) 
{
  // silently does nothing when an invalid timeoutId (should be a Timeout instance or an int32 value) is passed in
  if (!(timeoutId instanceof Timeout) && !Number.isInteger(timeoutId))
    return;

  return cancelByTimeoutId(Number(timeoutId));
}

// expose the `Timeout` class
setTimeout.Timeout = Timeout;

if (!globalThis.setTimeout)
  globalThis.setTimeout = setTimeout;
if (!globalThis.clearTimeout)
  globalThis.clearTimeout = clearTimeout;

exports.setTimeout = setTimeout;
exports.clearTimeout = clearTimeout;
