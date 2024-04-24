/**
 * @file     internal-binding.d.ts
 * @author   Tom Tang <xmader@distributive.network>
 * @date     June 2023
 * 
 * @copyright Copyright (c) 2023 Distributive Corp.
 */

/**
 * Note: `internalBinding` APIs are generally unsafe as they do not perform argument type checking, etc.
 *       Argument checking should be done on the JavaScript side.
 */
declare function internalBinding(namespace: string): any; // catch-all

declare function internalBinding(namespace: "utils"): {
  defineGlobal(name: string, value: any): void;

  isAnyArrayBuffer(obj: any): obj is (ArrayBuffer | SharedArrayBuffer);
  isPromise<T>(obj: any): obj is Promise<T>;
  isRegExp(obj: any): obj is RegExp;
  isTypedArray(obj: any): obj is TypedArray;

  /**
   * Get the promise state (fulfilled/rejected/pending) and result (either fulfilled resolution or rejection reason)
   */
  getPromiseDetails(promise: Promise<any>): [state: PromiseState.Pending] | [state: PromiseState.Fulfilled | PromiseState.Rejected, result: any];

  /**
   * Get the proxy target object and handler
   * @return `undefined` if it's not a proxy
   */
  getProxyDetails<T extends object>(proxy: T): undefined | [target: T, handler: ProxyHandler<T>];
};

declare type TimerDebugInfo = object;

declare function internalBinding(namespace: "timers"): {
  /**
   * internal binding helper for the `setTimeout`/`setInterval` global functions
   * 
   * **UNSAFE**, does not perform argument type checks
   * 
   * @param repeat The call is to `setInterval` if true
   * @return timeoutId
   */
  enqueueWithDelay(handler: Function, delaySeconds: number, repeat: boolean, debugInfo?: TimerDebugInfo): number;

  /**
   * internal binding helper for the `clearTimeout` global function
   */
  cancelByTimeoutId(timeoutId: number): void;

  /**
   * internal binding helper for if a timer object has been ref'ed
   */
  timerHasRef(timeoutId: number): boolean;

  /**
   * internal binding helper for ref'ing the timer
   */
  timerAddRef(timeoutId: number): void;

  /**
   * internal binding helper for unref'ing the timer
   */
  timerRemoveRef(timeoutId: number): void;

  /**
   * Retrieve debug info inside the timer for the WTFPythonMonkey tool
   */
  getDebugInfo(timeoutId: number): TimerDebugInfo;

  /**
   * Retrieve the debug info for all timers that are still ref'ed
   */
  getAllRefedTimersDebugInfo(): TimerDebugInfo[];
};

export = internalBinding;
