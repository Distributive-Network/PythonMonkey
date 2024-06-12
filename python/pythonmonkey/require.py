# @file         require.py
#               Implementation of CommonJS "require" for PythonMonkey. This implementation uses the
#               ctx-module npm package to do the heavy lifting. That package makes a complete module
#               system, ostensibly in a separate context, but our implementation here reuses the
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
#               have simply made global variables of the form xyzModule where xyz is the module
#               identifier, and injected a require function which understands this. A better
#               implementation in Python that doesn't leak global symbols should be possible once
#               some PythonMonkey bugs are fixed.
#
#               Module Context Summary
#               - what CommonJS modules you can access depends on what require symbol you have
#               - pm means the pythonmonkey Python module exports
#               - pm.bootstrap.require is a special require that knows about modules which are not stored
#                 on disk. This is important, because ctx-module needs some modules, and this is how we
#                 solve the chicken<>egg problem.
#               - There is a node_modules folder inside the pminit module that comes with pythonmonkey.
#                 This folder is managed by npm by invoking `pminit npm`. These modules are intended as
#                 global modules, analogously to require('process') in nodejs.
#               - pm.require is a require that works as expected from python source code; i.e. relative
#                 modules are resolved relative to the python source code.
#               - pm.require has access to the pminit node_modules modules but not the bootstrap modules.
#               - The builtin_modules directory is available anywhere the pminit node_modules directory
#                 is, and has a higher precedence.
#
# @author       Wes Garland, wes@distributive.network
# @date         May 2023
#
# @copyright Copyright (c) 2023-2024 Distributive Corp.

import sys
import os
import io
from typing import Union, Dict, Literal, List
import importlib
import importlib.util
from importlib import machinery
import inspect
import functools

from . import pythonmonkey as pm

node_modules = os.path.abspath(
  os.path.join(
    importlib.util.find_spec("pminit").submodule_search_locations[0],  # type: ignore
    "..",
    "pythonmonkey",
    "node_modules"
  )
)
evalOpts = {'filename': __file__, 'fromPythonFrame': True}  # type: pm.EvalOptions

# Force to use UTF-8 encoding
# Windows may use other encodings / code pages that have many characters missing/unrepresentable
if isinstance(sys.stdin, io.TextIOWrapper):
  sys.stdin.reconfigure(encoding='utf-8')
if isinstance(sys.stdout, io.TextIOWrapper):
  sys.stdout.reconfigure(encoding='utf-8')
if isinstance(sys.stderr, io.TextIOWrapper):
  sys.stderr.reconfigure(encoding='utf-8')

# Add some python functions to the global python object for code in this file to use.
globalThis = pm.eval("globalThis;", evalOpts)
pm.eval("globalThis.python = { pythonMonkey: {}, stdout: {}, stderr: {} }", evalOpts)
globalThis.pmEval = pm.eval
globalThis.python.pythonMonkey.dir = os.path.dirname(__file__)
globalThis.python.pythonMonkey.isCompilableUnit = pm.isCompilableUnit
globalThis.python.pythonMonkey.nodeModules = node_modules
globalThis.python.print = print
globalThis.python.stdout.write = lambda s: sys.stdout.write(s)
globalThis.python.stderr.write = lambda s: sys.stderr.write(s)
globalThis.python.stdout.read = lambda n: sys.stdout.read(n)
globalThis.python.stderr.read = lambda n: sys.stderr.read(n)
globalThis.python.eval = eval
globalThis.python.exec = exec
globalThis.python.getenv = os.getenv
globalThis.python.paths = sys.path

globalThis.python.exit = pm.eval("""'use strict';
(exit) => function pythonExitWrapper(exitCode) {
  if (typeof exitCode === 'undefined')
    exitCode = pythonExitWrapper.code;
  if (typeof exitCode == 'undefined')
    exitCode = 0n;
  if (typeof exitCode === 'number')
    exitCode = BigInt(Math.floor(exitCode));
  if (typeof exitCode !== 'bigint')
    exitCode = 1n;
  exit(exitCode);
}
""", evalOpts)(sys.exit)

# bootstrap is effectively a scoping object which keeps us from polluting the global JS scope.
# The idea is that we hold a reference to the bootstrap object in Python-load, for use by the
# innermost code in ctx-module, without forcing ourselves to expose this minimalist code to
# userland-require
bootstrap = pm.eval("""
'use strict'; (function IIFE(python) {

const bootstrap = {
  modules: {
    vm: {},
    'ctx-module': {},
  },
}

/* ctx-module needs require() when it loads that can find vm and fs */
bootstrap.require = function bootstrapRequire(mid)
{
  if (bootstrap.modules.hasOwnProperty(mid))
    return bootstrap.modules[mid];

  if (bootstrap.modules['ctx-module'].CtxModule)
    return bootstrap.requireFromDisk(mid);

  const error = new Error('module not found: ' + mid);
  error = 'MODULE_NOT_FOUND';
  throw error;
}

bootstrap.modules.vm.runInContext = function runInContext(code, _unused_contextifiedObject, options)
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
 * debugging statements to stdout. This is quick implementation of the node built-in.
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
          var output;
          if (!bootstrap.inspect)
            output = Array.from(arguments).join(' ');
          else
            output = Array.from(arguments).map(x => {
              if (typeof x === 'string' || x instanceof String)
                return x;
              return bootstrap.inspect(x);
            }).join(' ');

          output = output.split('\\n');
          python.print(`${colour}${selector}${noColour} ` + output[0]);
          const spaces = ''.padEnd(selector.length + 1, ' ');
          for (let i=1; i < output.length; i++)
            python.print(spaces + output[i]);
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

return bootstrap;
})(globalThis.python)""", evalOpts)


def statSync_inner(filename: str) -> Union[Dict[str, int], bool]:
  """
  Inner function for statSync.

  Returns:
      Union[Dict[str, int], False]: The mode of the file or False if the file doesn't exist.
  """
  from os import stat
  filename = os.path.normpath(filename)
  if (os.path.exists(filename)):
    sb = stat(filename)
    return {'mode': sb.st_mode}
  else:
    return False


def readFileSync(filename, charset) -> str:
  """
  Utility function for reading files.
  Returns:
      str: The contents of the file
  """
  filename = os.path.normpath(filename)
  with open(filename, "r", encoding=charset) as fileHnd:
    return fileHnd.read()


def existsSync(filename: str) -> bool:
  filename = os.path.normpath(filename)
  return os.path.exists(filename)


bootstrap.modules.fs.statSync_inner = statSync_inner
bootstrap.modules.fs.readFileSync = readFileSync
bootstrap.modules.fs.existsSync = existsSync

# Read ctx-module module from disk and invoke so that this file is the "main module" and ctx-module has
# require and exports symbols injected from the bootstrap object above. Current PythonMonkey bugs
# prevent us from injecting names properly so they are stolen from trail left behind in the global
# scope until that can be fixed.
#
# lineno should be -5 but jsapi 102 uses unsigned line numbers, so we take the newlines out of the
# wrapper prologue to make stack traces line up.
with open(node_modules + "/ctx-module/ctx-module.js", "r") as ctxModuleSource:
  initCtxModule = pm.eval("""'use strict';
(function moduleWrapper_forCtxModule(require, exports)
{
""" + ctxModuleSource.read() + """
})
""", {'filename': node_modules + "/ctx-module/ctx-module.js", 'lineno': 0})
initCtxModule(bootstrap.require, bootstrap.modules['ctx-module'])


def load(filename: str) -> Dict:
  """
  Loads a python module using the importlib machinery sourcefileloader, prefills it with an exports object and returns
  the module.
  If the module is already loaded, returns it.

  Args:
      filename (str): The filename of the python module to load.

  Returns:
      : The loaded python module
  """

  filename = os.path.normpath(filename)
  name = os.path.basename(filename)
  if name not in sys.modules:
    sourceFileLoader = machinery.SourceFileLoader(name, filename)
    spec: machinery.ModuleSpec = importlib.util.spec_from_loader(
      sourceFileLoader.name, sourceFileLoader)  # type: ignore
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    module.exports = {}  # type: ignore
    spec.loader.exec_module(module)  # type: ignore
  else:
    module = sys.modules[name]
  return module.exports


globalThis.python.load = load

createRequireInner = pm.eval("""'use strict';(
/**
 * Factory function which returns a fresh 'require' function. The module cache will inherit from
 * globalThis.require, assuming it has been defined.
 *
 * @param {string} filename      the filename of the module that would get this require
 * @param {object} bootstrap     the bootstrap context; python imports, modules, etc, that are invisible
 *                               to normal JS
 * @param {string} extraPaths    colon-delimited list of paths to add to require.path
 * @param {boolean} isMain       true if the module is to be used as a program module
 *
 * @returns {function} require
 */
function createRequireInner(filename, bootstrap, extraPaths, isMain)
{
  const CtxModule = bootstrap.modules['ctx-module'].CtxModule;
  const moduleCache = globalThis.require?.cache || {};

  function loadPythonModule(module, filename)
  {
    module.exports = python.load(filename);
  }

  // TODO - find a better way to deal with Windows paths
  if (filename)
    filename = filename.split('\\\\').join('/');
  if (moduleCache[filename])
    return moduleCache[filename].require;

  const module = new CtxModule(globalThis, filename, moduleCache);
  if (!filename)
    module.paths = [];   /* fully virtual module - no module.path or module.paths */
  else
  {
    moduleCache[filename] = module;
    for (let path of Array.from(python.paths))
      module.paths.push(path + '/node_modules');
  }

  module.require.path.push(python.pythonMonkey.dir + '/builtin_modules');
  module.require.path.push(python.pythonMonkey.nodeModules);

  /* Add a .py loader, making it the first extension to be enumerated so py modules take precedence */
  const extCopy = Object.assign({}, module.require.extensions);
  for (let ext in module.require.extensions)
    delete module.require.extensions[ext];
  module.require.extensions['.py'] = loadPythonModule;
  Object.assign(module.require.extensions, extCopy);

  if (isMain)
  {
    globalThis.module = module;
    globalThis.exports = module.exports;
    globalThis.require = module.require;
    globalThis.require.main = module;
    module.loaded = true;
    moduleCache[filename] = module;
  }

  if (extraPaths)
    module.require.path.splice(module.require.path.length, 0, ...(extraPaths.split(',')));

  return module.require;
})""", evalOpts)

# API: pm.createRequire
# We cache the return value of createRequire to always use the same require for the same filename


def createRequire(filename, extraPaths: Union[List[str], Literal[False]] = False, isMain=False):
  """
  returns a require function that resolves modules relative to the filename argument.
  Conceptually the same as node:module.createRequire().

  example:
  from pythonmonkey import createRequire
  require = createRequire(__file__)
  require('./my-javascript-module')
  """
  fullFilename: str = os.path.abspath(filename)
  if (extraPaths):
    extraPathsStr = ':'.join(extraPaths)
  else:
    extraPathsStr = ''
  return createRequireInner(fullFilename, bootstrap, extraPathsStr, isMain)


bootstrap.requireFromDisk = createRequireInner(None, bootstrap, '', False)
bootstrap.inspect = bootstrap.requireFromDisk('util').inspect

# API: pm.runProgramModule


def runProgramModule(filename, argv, extraPaths=[]):
  """
  Run the program module. This loads the code from disk, sets up the execution environment, and then
  invokes the program module (aka main module). The program module is different from other modules in that
  1. it cannot return (must throw)
  2. the outermost block scope is the global scope, effectively making its scope a super-global to
     other modules
  """
  fullFilename = os.path.abspath(filename)
  createRequire(fullFilename, extraPaths, True)
  globalThis.__filename = fullFilename
  globalThis.__dirname = os.path.dirname(fullFilename)
  with open(fullFilename, encoding="utf-8", mode="r") as mainModuleSource:
    pm.eval(mainModuleSource.read(), {'filename': fullFilename, 'noScriptRval': True})
    # forcibly run in file mode. We shouldn't be getting the last expression of the script as the result value.

# The pythonmonkey require export. Every time it is used, the stack is inspected so that the filename
# passed to createRequire is correct. This is necessary so that relative requires work. If the filename
# found on the stack doesn't exist, we assume we're in the REPL or something and simply use the current
# directory as the location of a virtual module for relative require purposes.
#
# todo: instead of cwd+__main_virtual__, use a full pathname which includes the directory that the
#       running python program is in.
#


def require(moduleIdentifier: str):
  filename = inspect.stack()[1].filename
  if not os.path.exists(filename):
    filename = os.path.join(os.getcwd(), "__main_virtual__")
  return createRequire(filename)(moduleIdentifier)


# Restrict what symbols are exposed to the pythonmonkey module.
__all__ = ["globalThis", "require", "createRequire", "runProgramModule", "bootstrap"]
