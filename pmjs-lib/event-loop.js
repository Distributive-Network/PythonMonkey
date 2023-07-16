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

const references = new Set();
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
}

Timeout.prototype.toString = function eventLoop$$Timeout$toString()
{
  return `${this.id}` 
}

Timeout.prototype.unref = function eventLoop$$Timeout$unref()
{
  references.delete(this);
  this._refed = false;
}

Timeout.prototype.ref = function eventLoop$$Timeout$unref()
{
  console.log('add reference')
  references.add(this);
  this._refed = true;
}

/**
 * setTimeout method. setImmediate and setInterval are implemented in terms of setTimeout.
 */
function eventLoop$$setTimeout(callback, delayMs, ...args)
{
  var pyHnd, timer;

  if (delayMs)
    pyHnd = enqueueWithDelay(timerCallbackWrapper, Math.max(4, delayMs) / 1000);
  else
    pyHnd = enqueue(timerCallbackWrapper);
  
  timer = new Timeout(pyHnd);

  function timerCallbackWrapper()
  {
    if (timer._destroyed === true)
      return;
    callback.apply(this, ...args);
    if (timer._repeat && typeof timer._repeat === 'number')
      enqueueWithDelay(callbackWrapper, Math.max(4, delayMs) / 1000, timer._repeat);
    else
      timer.unref();

    if (references.size === 0)
      endLoop();
  }
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

function eventLoop$$clearTimeout(timer)
{
  timer.unref();
  timer._repeat = false;
  timer._destroyed = true;
}

exports.setInterval    = eventLoop$$setImmediate;
exports.setTimeout     = eventLoop$$setTimeout;
exports.setInterval    = eventLoop$$setInterval;
exports.clearTimeout   = eventLoop$$clearTimeout;
exports.clearInterval  = eventLoop$$clearTimeout;
exports.clearImmediate = eventLoop$$clearTimeout;
