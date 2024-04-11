/**
 * @file     global.d.ts
 * @author   Tom Tang <xmader@distributive.network>
 * @date     May 2023
 * 
 * @copyright Copyright (c) 2023 Distributive Corp.
 */

declare const python: {
  pythonMonkey: {
    /** root directory of the pythonmonkey package */
    dir: string;
  };
  /** Python `print` */
  print(...values: any): void;
  /** Python `eval` */
  eval(code: string, globals?: Record<string, any>, locals?: Record<string, any>): any;
  /** Python `exec` */
  exec(code: string, globals?: Record<string, any>, locals?: Record<string, any>): void;
  /** Python `sys.stdout`. */
  stdout: {
    /** Write the given string to stdout. */
    write(s: string): number;
    read(n: number): string;
  };
  /** Python `sys.stderr`. */
  stderr: {
    /** Write the given string to stderr. */
    write(s: string): number;
    read(n: number): string;
  };
  /** Python `os.getenv`. Get an environment variable, return undefined if it doesn't exist. */
  getenv(key: string): string | undefined;
  /** Python `exit`. Exit the program. */
  exit(exitCode: number): never;
  /** Loads a python module using importlib, prefills it with an exports object and returns the module. */
  load(filename: string): object;
  /** Python `sys.path` */
  paths: string[];
};

declare var __filename: string;
declare var __dirname: string;

/** see `pm.eval` */
declare function pmEval(code: string): any;

// Expose our own `console` as a property of the global object
// XXX: ↓↓↓ we must use "var" here
declare var console: import("console").Console;

// Expose `atob`/`btoa` as properties of the global object
declare var atob: typeof import("base64").atob;
declare var btoa: typeof import("base64").btoa;

// Expose `setTimeout`/`clearTimeout` APIs
declare var setTimeout: typeof import("timers").setTimeout;
declare var clearTimeout: typeof import("timers").clearTimeout;
// Expose `setInterval`/`clearInterval` APIs
declare var setInterval: typeof import("timers").setInterval;
declare var clearInterval: typeof import("timers").clearInterval;

// Expose `URL`/`URLSearchParams` APIs
declare var URL: typeof import("url").URL;
declare var URLSearchParams: typeof import("url").URLSearchParams;

// Expose `XMLHttpRequest` (XHR) API
declare var XMLHttpRequest: typeof import("XMLHttpRequest").XMLHttpRequest;

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
