/**
 * @file     console.js
 * @author   Tom Tang <xmader@distributive.network>
 * @date     June 2023
 * 
 * @copyright Copyright (c) 2023 Distributive Corp.
 */
const { customInspectSymbol, format } = require('util');

/** @typedef {(str: string) => void} WriteFn */
/** @typedef {{ write: WriteFn }} IOWriter */

/**
 * @see https://developer.mozilla.org/en-US/docs/Web/API/Console_API
 * @see https://console.spec.whatwg.org/
 */
class Console 
{
  /** @type {WriteFn} */
  #writeToStdout;
  /** @type {WriteFn} */
  #writeToStderr;

  /**
   * @type {{ [label: string]: number; }}
   * @see https://console.spec.whatwg.org/#counting
   */
  #countMap = {};

  #groupLevel = 0;

  /**
   * @type {{ [label: string]: number; }}
   * @see https://console.spec.whatwg.org/#timing
   */
  #timerTable = {};

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

    this.log   = (...args) => void this.#writeToStdout(this.#formatToStr(...args));
    this.debug = (...args) => void this.#writeToStdout(this.#formatToStr(...args));
    this.info  = (...args) => void this.#writeToStdout(this.#formatToStr(...args));
    this.warn  = (...args) => void this.#writeToStderr(this.#formatToStr(...args));
    this.error = (...args) => void this.#writeToStderr(this.#formatToStr(...args));

    this.clear = () => void this.#writeToStdout('\x1bc'); // clear the terminal, see https://stackoverflow.com/questions/47503734

    this.assert = (condition, ...data) => // See https://console.spec.whatwg.org/#assert
    {
      if (condition) return; // step 1

      const message = 'Assertion failed'; // step 2
      if (data.length === 0) // step 3
        data.push(message);
      else // step 4
      {
        const first = data[0]; // step 4.1
        if (typeof first !== 'string') data.unshift(message+':'); // step 4.2
        else data[0] = `${message}: ${first}`; // step 4.3
      }

      return this.error(...data); // print out
    };

    this.trace = (...args) => // implement console.trace using new Error().stack
    {
      const header = args.length > 0
        ? `Trace: ${format(...args)}\n`
        : 'Trace\n';

      let stacks = new Error().stack
            .split('\n')
      stacks.shift();              // skip the first line which is this.trace itself
      stacks = stacks
            .filter(s => s !== '') // filter out empty lines
            .map(s => '    '+s)    // add indent
            .join('\n');
      return this.debug(header + stacks);
    };

    // TODO (Tom Tang): implement those properly instead of aliases to console.log
    this.dir = this.log;
    this.dirxml = this.log;
    this.table = this.log;

    // Counting functions
    // @see https://console.spec.whatwg.org/#count
    this.count = (label = 'default') =>
    {
      if (this.#countMap[label])
        this.#countMap[label] += 1;
      else
        this.#countMap[label] = 1;
      this.info(`${label}: ${this.#countMap[label]}`);
    };

    this.countReset = (label = 'default') =>
    {
      if (this.#countMap[label])
        this.#countMap[label] = 0;
      else
        this.warn(`Counter for '${label}' does not exist.`);
    };

    // Grouping functions
    // @see https://console.spec.whatwg.org/#grouping
    this.group = (...data) =>
    {
      if (data.length > 0)
        this.log(...data);
      this.#groupLevel++;
    };
    this.groupCollapsed = this.group;

    this.groupEnd = () =>
    {
      this.#groupLevel--;
      if (this.#groupLevel < 0)
        this.#groupLevel = 0;
    };

    // Timing functions
    // @see https://console.spec.whatwg.org/#timing
    this.time = (label = 'default') =>
    {
      if (this.#timerTable[label])
        this.warn(`Label '${label}' already exists for console.time()`);
      else
        this.#timerTable[label] = Date.now();
    };

    this.timeLog = (label = 'default', ...data) =>
    {
      if (!this.#timerTable[label])
        return this.warn(`No such label '${label}' for console.timeLog()`);

      const duration = Date.now() - this.#timerTable[label];
      data.unshift(`${label}: ${duration}ms`);

      return this.log(...data);
    };

    this.timeEnd = (label = 'default') =>
    {
      if (!this.#timerTable[label])
        return this.warn(`No such label '${label}' for console.timeEnd()`);

      const startTime = this.#timerTable[label];
      delete this.#timerTable[label];

      const duration = Date.now() - startTime;
      return this.info(`${label}: ${duration}ms`);
    };
  }

  /**
   * Format with appropriate grouping level
   * @return {string}
   */
  #formatToStr(...args) 
  {
    const msg = format(...args) + '\n';
    return msg.split('\n').map(s => '│   '.repeat(this.#groupLevel) + s).join('\n');
  }

  /**
   * Re-export the `Console` constructor as global `console.Console`, like in Node.js
   */
  get Console() 
  {
    return Console;
  }

  /**
   * Export the `nodejs.util.inspect.custom` symbol as a static property of `Console`
   */
  static customInspectSymbol = customInspectSymbol;
}

if (!globalThis.console) 
{
  globalThis.console = new Console(
    python.stdout /* sys.stdout */,
    python.stderr /* sys.stderr */
  );
}

exports.Console = Console;
