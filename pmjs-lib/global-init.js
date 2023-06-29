/**
 * @file        global-init.js
 *              Set up global scope which is used to run either program or REPL code in the pmjs script
 *              runner.
 *
 * @author      Wes Garland, wes@distributive.network
 * @date        June 2023
 */
'use strict';

require('console');
globalThis.require = require;
globalThis.module  = module;

/* Anything loaded with require() before the program started was a side effect and not part of the 
 * program. This means that by now, whoever needed the resources should have memoized them someplace
 * safe, and we can remove them to keep the namespace clean.
 */
for (let mid in require.cache)
  delete require.cache[mid];

/**
 * Set the global arguments array, which is just the program's argv.  We use an argvBuilder function to
 * get around PythonMonkey's missing list->Array coercion. /wg june 2023 
 */
exports.setArguments = function pmjsRequire$$init()
{
  const argv = [];
  globalThis.arguments = argv;
  return argvBuilder;

  function argvBuilder(arg)
  {
    globalThis.arguments.push(arg)
  }
}
