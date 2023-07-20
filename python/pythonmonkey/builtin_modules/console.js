/**
 * @file     console.js
 * @author   Tom Tang <xmader@distributive.network>
 * @date     June 2023
 */
const { customInspectSymbol, format } = require("util");

/** @typedef {(str: string) => void} WriteFn */
/** @typedef {{ write: WriteFn }} IOWriter */

/**
 * @see https://developer.mozilla.org/en-US/docs/Web/API/Console_API
 */
// TODO (Tom Tang): adhere https://console.spec.whatwg.org/
class Console {
  /** @type {WriteFn} */
  #writeToStdout;
  /** @type {WriteFn} */
  #writeToStderr;

  /**
   * Console constructor, form 1
   * @overload
   * @param {IOWriter} stdout - object with write method
   * @param {IOWriter} stderr - object with write method
   * @param {boolean=} ignoreErrors - currently unused in PythonMonkey
   * @see https://nodejs.org/api/console.html#new-consolestdout-stderr-ignoreerrors
   */
  /**
   * Console constructor, form 2
   * @overload
   * @param {ConsoleConstructorOptions} options - options object
   * @typedef {object} ConsoleConstructorOptions
   * @property {IOWriter} stdout - object with write method
   * @property {IOWriter} stderr - object with write method
   * @property {boolean=} ignoreErrors - currently unused in PythonMonkey
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

    this.log   = (...args) => this.#writeToStdout(this.#formatToStr(...args));
    this.debug = (...args) => this.#writeToStdout(this.#formatToStr(...args));
    this.info  = (...args) => this.#writeToStdout(this.#formatToStr(...args));
    this.warn  = (...args) => this.#writeToStderr(this.#formatToStr(...args));
    this.error = (...args) => this.#writeToStderr(this.#formatToStr(...args));
  }

  /**
   * @return {string}
   */
  #formatToStr(...args) {
    return format(...args) + "\n"
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

if (!globalThis.console) {
  globalThis.console = new Console(
    python.stdout /* sys.stdout */,
    python.stderr /* sys.stderr */
  );
}

exports.Console = Console;
