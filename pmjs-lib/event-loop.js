/**
 * @file        event-loops.js
 *              Code for creating and manipulating a NodeJS-style reference-centric event loop for use
 *              within pmjs. The code currently in builtin_modules/timers.js implements WHATWG-spec-
 *              compliant timers, but this code needs different functionality and access to the Python
 *              awaitables implementing the timers because of that.
 *
 * @author      Wes Garland, wes@distributive.network
 * @date        July 2023
 */
'use strict';

const { enqueue, enqueueWithDelay, cancelTimer, endLoop } = require('./event-loop-asyncio');

const pendingTimers = new Set();
var seq = 0;

/**
 * Timeout constructor. This is the handle returned from setTimeout, setInterval, and setImmediate in an
 * environment with a node-style event-loop. We implement the following methods, modelled on Node.js:
 * - ref()
 * - unref()
 * - toString()
 *
 * Also like Node.js, there is a repeat property on this handle which controls if it is re-scheduled
 * every time it fires. This is the main difference between intervals and timeouts.
 * 
 * @param {Dict} pyHnd     a dict with a timer attribute which can be used for cancelling the timeout
 */
function Timeout(pyHnd)
{
  this.id = ++seq;
  this.pyHnd = pyHnd;
  this.repeat = false;
  this.ref();

  pendingTimers.add(this);
}

Timeout.prototype.toString = function eventLoop$$Timeout$toString()
{
  return `${this.id}` 
}

/**
 * Remove a reference from a timer. Program will not naturally exit until all references are removed.
 */
Timeout.prototype.unref = function eventLoop$$Timeout$unref()
{
  this._refed = false;
}

/**
 * Add a reference to a timer. Program will not naturally exit until all references are removed. Timers
 * are referenced by default. References are binary, on/off, not counted. 
 */
Timeout.prototype.ref = function eventLoop$$Timeout$unref()
{
  this._refed = true;
}

/**
 * setTimeout method. setImmediate and setInterval are implemented in terms of setTimeout.
 * @param {function} callback     Function to run after delay
 * @param {number}   delayMs      minimum number of ms to delay; false to run immediately
 * @param {...}      ...args      arguments passed to callback
 * @returns instance of Timer
 */
function eventLoop$$setTimeout(callback, delayMs, ...args)
{
  var pyHnd, timer;

  if (delayMs >= 0)
    pyHnd = enqueueWithDelay(timerCallbackWrapper, Math.max(4, delayMs) / 1000);
  else
    pyHnd = enqueue(timerCallbackWrapper);
  
  timer = new Timeout(pyHnd);

  function timerCallbackWrapper()
  {
    const globalInit = require('./global-init');

    if (timer._destroyed === true)
      return;

    try
    {
      const p = callback.apply(this, ...args);
      if (p instanceof Promise)
        p.catch(globalInit.unhandledRejectionHandler)
    }
    catch (error)
    {
      globalInit.uncaughtExceptionHandler(error);
    }

    if (timer._repeat && typeof timer._repeat === 'number')
      enqueueWithDelay(timerCallbackWrapper, Math.max(4, timer._repeat) / 1000);
    else
      timer.unref();

    maybeEndLoop();
  }

  return timer;
}

function eventLoop$$setInterval(callback, delayMs, ...args)
{
  const timer = eventLoop$$setTimeout(callback, delayMs, ...args)
  timer._repeat = delayMs;
  return timer;
}

function eventLoop$$setImmediate(callback, ...args)
{
  return setTimeout(callback, false, ...args)
}

/**
 * Remove a timeout/interval/immediate by cleaning up JS object and removing Timer from Python's event loop.
 */
function eventLoop$$clearTimeout(timer)
{
  if (timer._destroyed)
    return;
  timer.unref();
  timer._repeat = false;
  timer._destroyed = true;
  cancelTimer(timer.pyHnd);
}

/* Scan the pendingTimers for any event loop references. If there are none, clear all the pending timers
 * and end the event loop.
 */
function maybeEndLoop()
{
  for (let pendingTimer of pendingTimers)
    if (pendingTimer._refed === true)
      return;

  for (let pendingTimer of pendingTimers)
    eventLoop$$clearTimeout(pendingTimer);

  endLoop();
}

/**
 * Remove all references - part of a clean shutdown.
 */
function eventLoop$$unrefEverything()
{
  for (let pendingTimer of pendingTimers)
    pendingTimer.unref();
}

/* Enumerable exports are intended to replace global methods of the same name when running this style 
 * of event loop.
 */
exports.setImmediate   = eventLoop$$setImmediate;
exports.setTimeout     = eventLoop$$setTimeout;
exports.setInterval    = eventLoop$$setInterval;
exports.clearTimeout   = eventLoop$$clearTimeout;
exports.clearInterval  = eventLoop$$clearTimeout;
exports.clearImmediate = eventLoop$$clearTimeout;

/* Not enumerable -> not part of official/public API */
Object.defineProperty(exports, 'unrefEverything', {
  value: eventLoop$$unrefEverything,
  enumerable: false
});
