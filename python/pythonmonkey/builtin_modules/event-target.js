/**
 * @file     event-target.js
 *           Implement browser-style EventTarget
 * @see      https://dom.spec.whatwg.org/#eventtarget
 * @author   Tom Tang <xmader@distributive.network>
 * @date     August 2023
 */

/**
 * The Event interface represents an event which takes place in the DOM.
 * @see https://developer.mozilla.org/en-US/docs/Web/API/Event
 */
class Event
{
  /**
   * The name identifying the type of the event.
   */
  type = '';

  /**
   * @type {EventTarget}
   */
  target = null;

  /**
   * @param {string} type A string with the name of the event.
   */
  constructor(type)
  {
    this.type = type;
  }

  // TODO: to be implemented
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
    // Set the Event.target property to the current EventTarget
    event.target = this;

    const type = event.type;
    if (!type)
      return;

    // Call "on" methods
    if (typeof this['on' + type] === 'function')
      this['on' + type]();

    // Call listeners registered with addEventListener()
    if (!Object.hasOwn(this.#listeners, type))
      return;
    for (const listener of this.#listeners[type].values())
      if (typeof listener === 'function')
        listener(event);
      else
        listener.handleEvent(event);
  }
}

if (!globalThis.Event)
  globalThis.Event = Event;
if (!globalThis.EventTarget)
  globalThis.EventTarget = EventTarget;

exports.Event = Event;
exports.EventTarget = EventTarget;
