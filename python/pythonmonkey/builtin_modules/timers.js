/**
 * @file     timers.js
 * @author   Tom Tang <xmader@distributive.network>
 * @date     May 2023
 * 
 * @copyright Copyright (c) 2023 Distributive Corp.
 */
const internalBinding = require('internal-binding');

const { 
  enqueueWithDelay,
  cancelByTimeoutId,
  timerHasRef,
  timerAddRef,
  timerRemoveRef,
} = internalBinding('timers');

const { DOMException } = require('dom-exception');

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
   * Sets the timer's start time to the current time, 
   * and reschedules the timer to call its callback at the previously specified duration adjusted to the current time.
   * 
   * Using this on a timer that has already called its callback will reactivate the timer.
   */
  refresh()
  {
    throw new DOMException('Timeout.refresh() method is not supported by PythonMonkey.', 'NotSupportedError');
    // TODO: Do we really need to closely resemble the Node.js API?
    // This one is not easy to implement since we need to memorize the callback function and delay parameters in every `setTimeout`/`setInterval` call.
  }

  /**
   * Cancels the timeout.
   * @deprecated legacy Node.js API. Use `clearTimeout()` instead
   */
  close()
  {
    return clearTimeout(this);
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
 * Normalize the arguments to `setTimeout`,`setImmediate` or `setInterval`
 * @param {Function | string} handler
 * @param {number} delayMs timeout milliseconds
 * @param {any[]} additionalArgs additional arguments to be passed to the `handler`
 */
function _normalizeTimerArgs(handler, delayMs, additionalArgs)
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
  const boundHandler = handler.bind(thisArg, ...additionalArgs);

  // Get the delay time in seconds
  //  JS `setTimeout` takes milliseconds, but Python takes seconds
  delayMs = Number(delayMs) || 0; // ensure the `delayMs` is a `number`, explicitly do type coercion
  if (delayMs < 0)
    delayMs = 0; // as spec-ed
  const delaySeconds = delayMs / 1000; // convert ms to s

  // Populate debug information for the WTFPythonMonkey tool
  const stacks = new Error().stack.split('\n');
  const timerType = stacks[1]?.match(/^set(Timeout|Immediate|Interval)/)?.[0]; // `setTimeout@...`/`setImmediate@...`/`setInterval@...` is on the second line of stack trace
  const debugInfo = {
    type: timerType, // "setTimeout", "setImmediate", or "setInterval"
    fn: handler,
    args: additionalArgs,
    startTime: new Date(),
    delaySeconds,
    stack: stacks.slice(2).join('\n'), // remove the first line `_normalizeTimerArgs@...` and the second line `setTimeout/setImmediate/setInterval@...`
  };

  return { boundHandler, delaySeconds, debugInfo };
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
  const { boundHandler, delaySeconds, debugInfo } = _normalizeTimerArgs(handler, delayMs, args);
  return new Timeout(enqueueWithDelay(boundHandler, delaySeconds, false, debugInfo));
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

/**
 * Implement the `setImmediate` global function  
 * **NON-STANDARD**, for Node.js compatibility only.
 * @see https://developer.mozilla.org/en-US/docs/Web/API/Window/setImmediate and
 * @see https://nodejs.org/en/learn/asynchronous-work/understanding-setimmediate
 * @param {Function | string} handler
 * @param {any[]} args additional arguments to be passed to the `handler`
 */
function setImmediate(handler, ...args)
{
  // setImmediate is equal to setTimeout with a 0ms delay
  const { boundHandler, debugInfo } = _normalizeTimerArgs(handler, 0, args);
  return new Timeout(enqueueWithDelay(boundHandler, 0, false, debugInfo));
}

/**
 * Implement the `clearImmediate` global function  
 * **NON-STANDARD**, for Node.js compatibility only.
 * @alias to `clearTimeout`
 * @see https://developer.mozilla.org/en-US/docs/Web/API/Window/clearImmediate
 */
const clearImmediate = clearTimeout;

/**
 * Implement the `setInterval` global function
 * @see https://developer.mozilla.org/en-US/docs/Web/API/setInterval and
 * @see https://html.spec.whatwg.org/multipage/timers-and-user-prompts.html#dom-setinterval
 * @param {Function | string} handler
 * @param {number} delayMs timeout milliseconds, use value of 0 if this is omitted
 * @param {any[]} args additional arguments to be passed to the `handler`
 * @return {Timeout} timeoutId
 */
function setInterval(handler, delayMs = 0, ...args) 
{
  const { boundHandler, delaySeconds, debugInfo } = _normalizeTimerArgs(handler, delayMs, args);
  return new Timeout(enqueueWithDelay(boundHandler, delaySeconds, true, debugInfo));
}

/**
 * Implement the `clearInterval` global function
 * @alias to `clearTimeout`
 * @see https://developer.mozilla.org/en-US/docs/Web/API/clearInterval
 */
const clearInterval = clearTimeout;

// expose the `Timeout` class
setTimeout.Timeout = Timeout;
setImmediate.Timeout = Timeout;
setInterval.Timeout = Timeout;

if (!globalThis.setTimeout)
  globalThis.setTimeout = setTimeout;
if (!globalThis.clearTimeout)
  globalThis.clearTimeout = clearTimeout;

if (!globalThis.setImmediate)
  globalThis.setImmediate = setImmediate;
if (!globalThis.clearImmediate)
  globalThis.clearImmediate = clearImmediate;

if (!globalThis.setInterval)
  globalThis.setInterval = setInterval;
if (!globalThis.clearInterval)
  globalThis.clearInterval = clearInterval;

exports.setTimeout = setTimeout;
exports.clearTimeout = clearTimeout;
exports.setImmediate = setImmediate;
exports.clearImmediate = clearImmediate;
exports.setInterval = setInterval;
exports.clearInterval = clearInterval;
