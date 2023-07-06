/**
 * @file     console.js
 *           Temporary implementation of console.log etc.
 * @author   Wes Garland, wes@distributive.network
 * @date     June 2023
 */
function Console(stdout, stderr)
{
  function write(stream, args)
  {
    stream.write(args.join(' ') + '\n');
  }
    
  if (typeof stdout === 'object' && !stdout.write)
  {
    /* handle overload, eg new Console(process) */
    stderr = stdout.stderr;
    stdout = stdout.stdout;
  }

  this.log   = (...args) => write(stdout, args);
  this.debug = (...args) => write(stdout, args);
  this.error = (...args) => write(stderr, args);
  this.warn  = (...args) => write(stderr, args);
  this.info  = (...args) => write(stdout, args);
}

if (!globalThis.console)
  globalThis.console = new Console(python)

exports.Console = Console;
