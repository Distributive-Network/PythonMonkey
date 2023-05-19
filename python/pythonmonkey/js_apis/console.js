// @ts-check
///<reference path="./global.d.ts"/>

const { defineGlobal } = internalBinding("utils")

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
   * @param {WriteFn} writeToStdout 
   * @param {WriteFn} writeToStderr 
   */
  constructor(writeToStdout, writeToStderr) {
    this.#writeToStdout = writeToStdout
    this.#writeToStderr = writeToStderr
  }

  /**
   * @return {string}
   */
  #formatToStr(...args) {
    // TODO (Tom Tang): pretty print Arrays & Objects 
    return args.join(" ") + "\n"
  }

  log(...args) {
    this.#writeToStdout(this.#formatToStr(...args))
  }

  warn(...args) {
    this.#writeToStderr(this.#formatToStr(...args))
  }

  // TODO (Tom Tang): implement more methods
}

// https://github.com/nodejs/node/blob/v20.1.0/lib/internal/console/constructor.js#L681-L685
Console.prototype.debug = Console.prototype.log;
Console.prototype.info = Console.prototype.log;
Console.prototype.error = Console.prototype.warn;

defineGlobal("console", new Console(
  pythonBindings[0] /* sys.stdout.write */,
  pythonBindings[1] /* sys.stderr.write */
))
