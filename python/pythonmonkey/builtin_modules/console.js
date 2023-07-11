/**
 * @file     console.js
 * @author   Tom Tang <xmader@distributive.network>
 * @date     June 2023
 */
const { customInspectSymbol, format } = require("util");

/** @typedef {(str: string) => void} WriteFn */

/**
 * @see https://developer.mozilla.org/en-US/docs/Web/API/Console_API
 */
// TODO (Tom Tang): It's easier to copy implementations from Node.js version 8 than Node.js 20,
//                  see https://github.com/nodejs/node/blob/v8.17.0/lib/console.js
// TODO (Tom Tang): adhere https://console.spec.whatwg.org/
class Console {
  /** @type {WriteFn} */
  #writeToStdout;
  /** @type {WriteFn} */
  #writeToStderr;

  /**
   * Console constructor, form 1
   * @param {object} stdout - object with write method
   * @param {object} stderr - object with write method
   * @param {boolean} ignoreErrors - currently unused in PythonMonkey
   * @see https://nodejs.org/api/console.html#new-consolestdout-stderr-ignoreerrors
   */
  /**
   * Console constructor, form 2
   * @param {object} options - options object
   */
  constructor(stdout, stderr, ignoreErrors)
  {
    var options;

    if (arguments.length === 1)
      options = stdout;
    else
    {
      if (typeof ignoreErrors === 'undefined')
        ignoreErrors = true;
      options = { stdout, stderr, ignoreErrors };
    }
    this.#writeToStdout = options.stdout.write;
    this.#writeToStderr = options.stderr.write;
  }

  /**
   * @return {string}
   */
  #formatToStr(...args) {
    return format(...args) + "\n"
  }

  log(...args) {
    this.#writeToStdout(this.#formatToStr(...args))
  }

  warn(...args) {
    this.#writeToStderr(this.#formatToStr(...args))
  }

  // TODO (Tom Tang): implement more methods

  /**
   * Re-export the `Console` constructor as global `console.Console`, like in Node.js
   */
  get Console() {
    return Console
  }

  /**
   * Export the `nodejs.util.inspect.custom` symbol as a static property of `Console`
   */
  static customInspectSymbol = customInspectSymbol;
}

// https://github.com/nodejs/node/blob/v20.1.0/lib/internal/console/constructor.js#L681-L685
Console.prototype.debug = Console.prototype.log;
Console.prototype.info = Console.prototype.log;
Console.prototype.error = Console.prototype.warn;

if (!globalThis.console) {
  globalThis.console = new Console(
    python.stdout /* sys.stdout */,
    python.stderr /* sys.stderr */
  );
}

exports.Console = Console;
