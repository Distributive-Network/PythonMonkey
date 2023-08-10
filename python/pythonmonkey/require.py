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
#               have simply made global variables of the form xyzModule where xyz is the module
#               identifier, and injected a require function which understands this. A better
#               implementation in Python that doesn't leak global symbols should be possible once
#               some PythonMonkey bugs are fixed.
#
# @author       Wes Garland, wes@distributive.network
# @date         May 2023
#

import sys, os, io
from typing import Union, Dict, Literal, List
import importlib
import importlib.util
from importlib import machinery
import inspect
import functools

from . import pythonmonkey as pm 

node_modules = os.path.abspath(
  os.path.join(
    importlib.util.find_spec("pminit").submodule_search_locations[0], # type: ignore
    "..",
    "pythonmonkey",
    "node_modules"
  )
)
evalOpts = { 'filename': __file__, 'fromPythonFrame': True } # type: pm.EvalOptions

# Force to use UTF-8 encoding
# Windows may use other encodings / code pages that have many characters missing/unrepresentable
if isinstance(sys.stdin,  io.TextIOWrapper): sys.stdin.reconfigure(encoding='utf-8')
if isinstance(sys.stdout, io.TextIOWrapper): sys.stdout.reconfigure(encoding='utf-8')
if isinstance(sys.stderr, io.TextIOWrapper): sys.stderr.reconfigure(encoding='utf-8')

# Add some python functions to the global python object for code in this file to use.
globalThis = pm.eval("globalThis;", evalOpts)
pm.eval("globalThis.python = { pythonMonkey: {}, stdout: {}, stderr: {} }", evalOpts);
globalThis.pmEval = pm.eval
globalThis.python.pythonMonkey.dir = os.path.dirname(__file__)
#globalThis.python.pythonMonkey.version = pm.__version__
#globalThis.python.pythonMonkey.module = pm
globalThis.python.pythonMonkey.isCompilableUnit = pm.isCompilableUnit
globalThis.python.pythonMonkey.nodeModules = node_modules
globalThis.python.print  = print
globalThis.python.stdout.write = lambda s: sys.stdout.write(s)
globalThis.python.stderr.write = lambda s: sys.stderr.write(s)
globalThis.python.stdout.read = lambda n: sys.stdout.read(n)
globalThis.python.stderr.read = lambda n: sys.stderr.read(n)
globalThis.python.eval = eval
globalThis.python.exec = exec
globalThis.python.getenv = os.getenv
globalThis.python.paths  = sys.path

globalThis.python.exit = pm.eval("""'use strict';
(exit) => function pythonExitWrapper(exitCode) {
  if (typeof exitCode === 'number')
    exitCode = BigInt(Math.floor(exitCode));
  exit(exitCode);
}
""", evalOpts)(sys.exit);

# bootstrap is effectively a scoping object which keeps us from polluting the global JS scope.
# The idea is that we hold a reference to the bootstrap object in Python-load, for use by the
# innermost code in ctx-module, without forcing ourselves to expose this minimalist code to
# userland-require.
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

  throw new Error('module not found: ' + mid);
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
        return { 'mode': sb.st_mode }
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
bootstrap.modules.fs.readFileSync   = readFileSync
bootstrap.modules.fs.existsSync     = existsSync

# Read ctx-module module from disk and invoke so that this file is the "main module" and ctx-module has
# require and exports symbols injected from the bootstrap object above. Current PythonMonkey bugs
# prevent us from injecting names properly so they are stolen from trail left behind in the global
# scope until that can be fixed.
#
# lineno should be -5 but jsapi 102 uses unsigned line numbers, so we take the newlines out of the
# wrapper prologue to make stack traces line up.
with open(node_modules + "/ctx-module/ctx-module.js", "r") as ctxModuleSource:
    initCtxModule = pm.eval("""'use strict';
(function moduleWrapper_forCtxModule(broken_require, broken_exports)
{
  const require = bootstrap.require;
  const exports = bootstrap.modules['ctx-module'];
""".replace("\n", " ") + "\n" + ctxModuleSource.read() + """
})
""", { 'filename': node_modules + "/ctx-module/ctx-module.js", 'lineno': 0 });
#broken initCtxModule(bootstrap.require, bootstrap.modules['ctx-module'].exports)
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

    filename = os.path.normpath(filename)
    name = os.path.basename(filename)
    if name not in sys.modules:
        sourceFileLoader = machinery.SourceFileLoader(name, filename)
        spec: machinery.ModuleSpec = importlib.util.spec_from_loader(sourceFileLoader.name, sourceFileLoader) # type: ignore
        module = importlib.util.module_from_spec(spec)
        sys.modules[name] = module
        module.exports = {} # type: ignore
        spec.loader.exec_module(module) # type: ignore
    else:
        module = sys.modules[name]
    return module.exports
globalThis.python.load = load

# API: pm.createRequire
# We cache the return value of createRequire to always use the same require for the same filename
@functools.lru_cache(maxsize=None) # unbounded function cache that won't remove any old values
def _createRequireInner(*args):
    return pm.eval("""'use strict';(
/**
 * Factory function which returns a fresh 'require' function. The module cache will inherit from
 * globalTHis.require, assuming it has been defined.
 *
 * @param {string} filename      the filename of the module that would get this require
 * @param {object} bootstrap     the bootstrap context; python imports that are invisible to normal JS
 * @param {string} extraPaths    colon-delimited list of paths to add to require.path
 * @param {boolean} isMain       true if the module is to be used as a program module
 *
 * @returns {function} require
 */
function createRequire(filename, bootstrap_broken, extraPaths, isMain)
{
  filename = filename.split('\\\\').join('/');
  const bootstrap = globalThis.bootstrap; /** @bug PM-65 */
  const CtxModule = bootstrap.modules['ctx-module'].CtxModule;
  const moduleCache = globalThis.require?.cache || {};

  function loadPythonModule(module, filename)
  {
    module.exports = python.load(filename);
  }

  if (moduleCache[filename])
    return moduleCache[filename].require;

  const module = new CtxModule(globalThis, filename, moduleCache);
  moduleCache[filename] = module;
  for (let path of Array.from(python.paths))
    module.paths.push(path + '/node_modules');
  module.require.path.push(python.pythonMonkey.dir + '/builtin_modules');
  module.require.path.push(python.pythonMonkey.nodeModules);
  module.require.extensions['.py'] = loadPythonModule;

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
})""", evalOpts)(*args)

def createRequire(filename, extraPaths: Union[List[str], Literal[False]] = False, isMain = False):
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
    return _createRequireInner(fullFilename, 'broken', extraPathsStr, isMain)

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
    globalThis.__filename = fullFilename;
    globalThis.__dirname = os.path.dirname(fullFilename);
    with open(fullFilename, encoding="utf-8", mode="r") as mainModuleSource:
        pm.eval(mainModuleSource.read(), {'filename': fullFilename})

def require(moduleIdentifier: str):
    # Retrieve the caller’s filename from the call stack
    filename = inspect.stack()[1].filename
    # From the REPL, the filename is "<stdin>", which is not a valid path
    if not os.path.exists(filename):
      filename = os.path.join(os.getcwd(), "__main__") # use the CWD instead
    return createRequire(filename)(moduleIdentifier)

# Restrict what symbols are exposed to the pythonmonkey module.
__all__ = ["globalThis", "require", "createRequire", "runProgramModule"]
