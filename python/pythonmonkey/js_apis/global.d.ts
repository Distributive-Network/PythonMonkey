/// <reference no-default-lib="true"/>
/// <reference lib="es2022" />

declare const pythonBindings: ReadonlyArray<any>;

/**
 * Note: `internalBinding` APIs are generally unsafe as they do not perform argument type checking, etc.
 *       Argument checking should be done in JavaScript side.
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

// Keep this in sync with both https://hg.mozilla.org/releases/mozilla-esr102/file/a03fde6/js/public/Promise.h#l331
//                        and  https://github.com/nodejs/node/blob/v20.2.0/deps/v8/include/v8-promise.h#L30
declare enum PromiseState { Pending = 0, Fulfilled = 1, Rejected = 2 }

declare type TypedArray =
  | Uint8Array
  | Uint8ClampedArray
  | Uint16Array
  | Uint32Array
  | Int8Array
  | Int16Array
  | Int32Array
  | Float32Array
  | Float64Array
  | BigUint64Array
  | BigInt64Array;
