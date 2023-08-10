/**
 * @file     timers.js
 * @author   Tom Tang <xmader@distributive.network>
 * @date     May 2023
 */
const internalBinding = require('internal-binding');

const { 
  enqueueWithDelay,
  cancelByTimeoutId
} = internalBinding('timers');

/**
 * Implement the `setTimeout` global function
 * @see https://developer.mozilla.org/en-US/docs/Web/API/setTimeout and
 * @see https://html.spec.whatwg.org/multipage/timers-and-user-prompts.html#dom-settimeout
 * @param {Function | string} handler
 * @param {number} delayMs timeout milliseconds, use value of 0 if this is omitted
 * @param {any[]} args additional arguments to be passed to the `handler`
 * @return {number} timeoutId
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

  return enqueueWithDelay(boundHandler, delaySeconds);
}

/**
 * Implement the `clearTimeout` global function
 * @see https://developer.mozilla.org/en-US/docs/Web/API/clearTimeout and
 * @see https://html.spec.whatwg.org/multipage/timers-and-user-prompts.html#dom-cleartimeout
 * @param {number} timeoutId
 * @return {void}
 */
function clearTimeout(timeoutId) 
{
  return cancelByTimeoutId(timeoutId);
}

if (!globalThis.setTimeout)
  globalThis.setTimeout = setTimeout;
if (!globalThis.clearTimeout)
  globalThis.clearTimeout = clearTimeout;

exports.setTimeout = setTimeout;
exports.clearTimeout = clearTimeout;
