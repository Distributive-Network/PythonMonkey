/**
 * @file     event-target.js
 *           Implement browser-style EventTarget
 * @see      https://dom.spec.whatwg.org/#eventtarget
 * @author   Tom Tang <xmader@distributive.network>
 * @date     August 2023
 * 
 * @copyright Copyright (c) 2023 Distributive Corp.
 */
'use strict';
const debug = globalThis.python.eval('__import__("pythonmonkey").bootstrap.require')('debug');

/**
 * The Event interface represents an event which takes place in the DOM.
 * @see https://developer.mozilla.org/en-US/docs/Web/API/Event
 */
class Event
{
  /**
   * Indicates whether the event bubbles up through the DOM tree or not.
   * @type {Boolean}
   */
  bubbles = false;

  /**
   * Indicates whether the event is cancelable.
   * @type {Boolean}
   */
  cancelable = true;

  /**
   * Indicates whether the event can bubble across the boundary between the shadow DOM and the regular DOM.
   * @type {Boolean}
   */
  composed = false;
  
  /**
   * The element to which the event handler has been attached.
   * @type {EventTarget}
   */
  currentTarget = null;

  /**
   * Indicates whether the call to event.preventDefault() canceled the event.
   * @type {Boolean}
   */
  devaultPrevented = false;

  /**
   * Indicates which phase of the event flow is currently being evaluated.
   * @type {Number}
   */
  eventPhase = Event.NONE;
  static NONE = 0;             // The event is not being processed
  static CAPTURING_PHASE = 1;  // The event is being propagated through the target's ancestor objects 
  static AT_TARGET = 2;        // The event has arrived at the event's target
  static BUBBLING_PHASE = 3;   // The event is propagating back up through the target's ancestors in reverse order, starting with the parent
  
  /**
   * Indicates whether the event was initiated by the browser or by a script.
   * @type {Boolean}
   */
  isTrusted = false;
  
  /**
   * A reference to the object to which the event was originally dispatched.
   * @type {EventTarget}
   */
  target = null;
  
  /**
   * The time at which the event was created (in milliseconds). By specification, this value is time since epoch.
   * @type {Number}
   */
  timeStamp = null;

  /**
   * The name identifying the type of the event.
   * @type {String} 
   */
  type = '';

  /**
   * @param {string} type A string with the name of the event.
   */
  constructor(type)
  {
    this.type = type;
  }

  // TODO: missing instance methods: Event.preventDefault(), Event.composedPath(), Event.stopImmediatePropagation(), Event.stopPropagation()
  // See https://developer.mozilla.org/en-US/docs/Web/API/Event#instance_methods
  // Also we need to figure out how we could handle event bubbling and capture in the PythonMonkey environment.
}

/**
 * @typedef {(ev: Event) => void} EventListenerFn
 * @typedef {{handleEvent(ev: Event): void}} EventListenerObj
 * @typedef {EventListenerFn | EventListenerObj} EventListener
 */

class EventTarget
{
  /**
   * @readonly
   * @type {{ [type: string]: Set<EventListener> }}
   */
  #listeners = Object.create(null);

  /**
   * Add an event listener that will be called whenever the specified event is delivered to the target
   * @param {string} type A case-sensitive string representing the event type to listen for
   * @param {EventListener | null} listener The object that receives a notification when an event of the specified type occurs
   */
  addEventListener(type, listener)
  {
    if (!listener)
      return;
    
    if (!Object.hasOwn(this.#listeners, type))
      this.#listeners[type] = new Set();
    
    this.#listeners[type].add(listener);
  }
  
  /**
   * Remove an event listener previously registered with EventTarget.addEventListener() from the target.
   * @param {string} type A string which specifies the type of event for which to remove an event listener.
   * @param {EventListener} listener The event listener function of the event handler to remove from the event target.
   */
  removeEventListener(type, listener)
  {
    if (Object.hasOwn(this.#listeners, type))
      this.#listeners[type].delete(listener);
  }

  /**
   * Send an Event to the target, (synchronously) invoking the affected event listeners in the appropriate order.
   * @param {Event} event The Event object to dispatch
   */
  dispatchEvent(event)
  {
    debug((event.debugTag || '') + 'events:dispatch')(event.constructor.name, event.type);
    // Set the Event.target property to the current EventTarget
    event.target = this;

    const type = event.type;
    if (!type)
      return;

    // Call "on" methods
    if (typeof this['on' + type] === 'function')
      this['on' + type].call(this, event);

    // Call listeners registered with addEventListener()
    if (!Object.hasOwn(this.#listeners, type))
      return;
    for (const listener of this.#listeners[type].values())
      if (typeof listener === 'function')
        listener.call(this, event);
      else
        listener.handleEvent.call(this, event);
  }

  /**
   * Determine whether the target has any kind of event listeners
   */
  _hasAnyListeners()
  {
    return Object.values(this.#listeners).some(t => t.size > 0);
  }

  /**
   * Determine whether the target has listeners of the given event type
   * @param {string} [type]
   */
  _hasListeners(type)
  {
    return this.#listeners[type] && this.#listeners[type].size > 0;
  }
}

if (!globalThis.Event)
  globalThis.Event = Event;
if (!globalThis.EventTarget)
  globalThis.EventTarget = EventTarget;

exports.Event = Event;
exports.EventTarget = EventTarget;
