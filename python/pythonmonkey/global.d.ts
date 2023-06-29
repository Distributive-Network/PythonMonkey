/**
 * @file     global.d.ts
 * @author   Tom Tang <xmader@distributive.network>
 * @date     May 2023
 */

declare const python: {
  pythonMonkey: {
    /** root directory of the pythonmonkey package */
    dir: string;
  };
  /** Python `print` */
  print(...values: any): void;
  /** Python `sys.stdout.write`. Write the given string to stdout. */
  stdout_write(s: string): void;
  /** Python `sys.stderr.write`. Write the given string to stderr. */
  stderr_write(s: string): void;
  /** Python `os.getenv`. Get an environment variable, return undefined if it doesn't exist. */
  getenv(key: string): string | undefined;
  /** Python `sys.path` */
  paths: string[];
};

/** see `pm.eval` */
declare function pmEval(code: string): any;

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
