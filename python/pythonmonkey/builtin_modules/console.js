/**
 * @file     console.js
 *           Temporary implementation of console.log etc.
 * @author   Wes Garland, wes@distributive.network
 * @date     June 2023
 */
function Console(print)
{
  this.log   = print;
  this.debug = print;
  this.error = print;
  this.warn  = print;
  this.info  = print;
}

if (!globalThis.console)
  globalThis.console = new Console(python.print);

exports.Console = Console;
