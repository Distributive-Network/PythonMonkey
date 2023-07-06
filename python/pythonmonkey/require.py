# @file         require.py
#               Implementation of CommonJS "require" for PythonMonkey. This implementation uses the
#               ctx-module npm package to do the heavy lifting. That package makes a complete module
#               system, obstensibly in a separate context, but our implementation here reuses the
#               PythonMonkey global context for both.
#
#               The context that ctx-module runs in needs a require function supporting
#                - require('debug') => returns a debug function which prints to the console when
#                                      debugging; see the node debug built-in
#                - require('fs')    => returns an object which has an implementation of readFileSync
#                                      and statSync. The implementation of statSync only needs to
#                                      return the mode member. The fs module also needs
#                                      constants.S_IFDIR available.
#                - require('vm')    => returns an object which has an implementation of evalInContext
#
#               In order to implement this basic require function for bootstrapping ctxModule, we
#               have simply made global variables of the form xxxModule where xxx is the module
#               identifier, and injected a require function which understands this. A better
#               implementation in Python that doesn't leak global symbols should be possible once
#               some PythonMonkey bugs are fixed.
#
# @author       Wes Garland, wes@distributive.network
# @date         May 2023
#

import sys, os, types
from typing import Union, Dict, Callable
import importlib
from importlib import machinery
from importlib import util

from . import pythonmonkey as pm 

# Add some python functions to the global python object for code in this file to use.
globalThis = pm.eval("globalThis;");
pm.eval("globalThis.python = { pythonMonkey: {} }");
globalThis.pmEval = pm.eval;
globalThis.python.print = print;
globalThis.python.stdout_write = sys.stdout.write;
globalThis.python.stderr_write = sys.stderr.write;
globalThis.python.getenv = os.getenv;
globalThis.python.pythonMonkey.dir = os.path.dirname(__file__);
globalThis.python.paths = ':'.join(sys.path);
pm.eval("python.paths = python.paths.split(':'); true"); # fix when pm supports arrays

# bootstrap is effectively a scoping object which keeps us from polluting the global JS scope.
# The idea is that we hold a reference to the bootstrap object in Python-load, for use by the
# innermost code in ctx-module, without forcing ourselves to expose this minimalist code to
# userland-require.
bootstrap = pm.eval("""
'use strict'; (function IIFE(python) {

const bootstrap = {
  modules: {
    vm: { runInContext: eval },
    'ctx-module': {},
  },
}

/* ctx-module needs require() when it loads that can find vm and fs */
bootstrap.require = function bootstrapRequire(mid)
{
  if (bootstrap.modules.hasOwnProperty(mid))
    return bootstrap.modules[mid];

  throw new Error('module not found: ' + mid);
}

bootstrap.modules.vm.runInContext_broken = function runInContext(code, _unused_contextifiedObject, options)
{
  var evalOptions = {};

  if (arguments.length === 2)
    options = arguments[2];

  if (options.filename)       evalOptions.filename = options.filename;
  if (options.lineOffset)     evalOptions.lineno   = options.lineOffset;
  if (options.columnOffset)   evalOptions.column   = options.columnOffset;

  return pmEval(code, evalOptions);
}

/**
 * The debug module has as its exports a function which may, depending on the DEBUG env var, emit
 * debugging statemnts to stdout. This is quick implementation of the node built-in.
 */
bootstrap.modules.debug = function debug(selector)
{
  var idx, colour;
  const esc = String.fromCharCode(27);
  const noColour = `${esc}[0m`;

  debug.selectors = debug.selectors || [];
  idx = debug.selectors.indexOf(selector);
  if (idx === -1)
  {
    idx = debug.selectors.length;
    debug.selectors.push(selector);
  }

  colour = `${esc}[${91 + ((idx + 1) % 6)}m`;
  const debugEnv = python.getenv('DEBUG');

  if (debugEnv)
  {
    for (let sym of debugEnv.split(' '))
    {
      const re = new RegExp('^' + sym.replace('*', '.*') + '$');
      if (re.test(selector))
      {
        return (function debugInner() {
          python.print(`${colour}${selector}${noColour} ` + Array.from(arguments).join(' '))
        });
      }
    }
  }

  /* no match => silent */
  return (function debugDummy() {});
}

/**
 * The fs module is like the Node.js fs module, except it only implements exactly what the ctx-module
 * module requires to load CommonJS modules. It is augmented heavily below by Python methods.
 */
bootstrap.modules.fs = {
  constants: { S_IFDIR: 16384 },
  statSync: function statSync(filename) {
    const ret = bootstrap.modules.fs.statSync_inner(filename);
    if (ret)
      return ret;

    const err = new Error('file not found: ' + filename); 
    err.code='ENOENT'; 
    throw err;
  },
};

/* Modules which will be available to all requires */
bootstrap.builtinModules = { debug: bootstrap.modules.debug };

/* temp workaround for PythonMonkey bug */
globalThis.bootstrap = bootstrap;

return bootstrap;
})(globalThis.python)""")

def statSync_inner(filename: str) -> Union[Dict[str, int], bool]:
    """
    Inner function for statSync.

    Returns:
        Union[Dict[str, int], False]: The mode of the file or False if the file doesn't exist.
    """
    from os import stat
    if (os.path.exists(filename)):
        sb = stat(filename)
        return { 'mode': sb.st_mode }
    else:
        return False

def readFileSync(filename, charset) -> str:
    """
    Utility function for reading files.
    Returns:
        str: The contents of the file
    """
    with open(filename, "r", encoding=charset) as fileHnd:
        return fileHnd.read()

bootstrap.modules.fs.statSync_inner = statSync_inner
bootstrap.modules.fs.readFileSync   = readFileSync
bootstrap.modules.fs.existsSync     = os.path.exists

# Read ctx-module module from disk and invoke so that this file is the "main module" and ctx-module has
# require and exports symbols injected from the bootstrap object above. Current PythonMonkey bugs
# prevent us from injecting names properly so they are stolen from trail left behind in the global
# scope until that can be fixed.
with open(os.path.dirname(__file__) + "/node_modules/ctx-module/ctx-module.js", "r") as ctxModuleSource:
    initCtxModule = pm.eval("""'use strict';
(function moduleWrapper_forCtxModule(broken_require, broken_exports)
{
  const require = bootstrap.require;
  const exports = bootstrap.modules['ctx-module'];
""" + ctxModuleSource.read() + """
})
""");
#broken initCtxModule(bootstrap.require, bootstrap.modules['ctx-module'].exports);
initCtxModule();

def load(filename: str) -> Dict:  
    """
    Loads a python module using the importlib machinery sourcefileloader, prefills it with an exports object and returns the module.
    If the module is already loaded, returns it.

    Args:
        filename (str): The filename of the python module to load.

    Returns:
        : The loaded python module
    """

    name = os.path.basename(filename)
    if name not in sys.modules:
        sourceFileLoader = machinery.SourceFileLoader(name, filename)
        spec = importlib.util.spec_from_loader(sourceFileLoader.name, sourceFileLoader)
        module = importlib.util.module_from_spec(spec)
        sys.modules[name] = module
        module.exports = {}
        spec.loader.exec_module(module)
    else:
        module = sys.modules[name]
    return module.exports
globalThis.python.load = load

"""
API - createRequire
returns a require function that resolves modules relative to the filename argument. 
Conceptually the same as node:module.createRequire().

example:
  from pythonmonkey import createRequire
  require = createRequire(__file__)
  require('./my-javascript-module')
"""
def createRequire(filename):
    createRequireInner = pm.eval("""'use strict';(
function createRequire(filename, bootstrap_broken)
{
  const bootstrap = globalThis.bootstrap; /** @bug PM-65 */
  const CtxModule = bootstrap.modules['ctx-module'].CtxModule;

  function loadPythonModule(module, filename)
  {
    module.exports = python.load(filename);
  }

  const module = new CtxModule(globalThis, filename, bootstrap.builtinModules);
  for (let path of python.paths)
    module.paths.push(path + '/node_modules');
  if (module.require.path.length === 0)
    module.require.path.push(python.pythonMonkey.dir + '/builtin_modules');
  module.require.extensions['.py'] = loadPythonModule;

  return module.require;
})""")
    return createRequireInner(filename)
