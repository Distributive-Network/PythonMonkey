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

import sys, warnings
import types
from typing import Union, Dict, Callable
import importlib
from importlib import machinery
from os import stat, path, getcwd, getenv

sys.path.append(path.dirname(__file__) + '/build/src')
warnings.filterwarnings("ignore", category=DeprecationWarning)

import pythonmonkey as pm

pm.eval("""
globalThis.python = {};
globalThis.global = globalThis;
globalThis.vmModule = { runInContext: eval };
globalThis.require = function outerRequire(mid) {
  const module = globalThis[mid + 'Module'];
  if (module)
    return module;
  throw new Error('module not found: ' + mid);
};
globalThis.debugModule = function debug(selector) {
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
};

function globalSet(name, prop)
{
  globalThis[name] = prop;
}

function propSet(objName, propName, propValue)
{
  globalThis[objName] = globalThis[objName] || {}; globalThis[objName][propName] = propValue;
}

""")

# globalSet and propSet are work-arounds until PythonMonkey correctly proxies objects.
globalSet = pm.eval("globalSet");
propSet = pm.eval("propSet")

# Add some python functions to the global python object for code in this file to use.
propSet('python', 'print', print);
propSet('python', 'getenv', getenv);
propSet('python', 'paths', ':'.join(sys.path));
pm.eval("python.paths = python.paths.split(':'); true"); # fix when pm supports arrays

# Implement enough of require('fs') so that ctx-module can find/load files

def statSync_inner(filename: str) -> Union[Dict[str, int], bool]:
    """
    Inner function for statSync.

    Returns:
        Union[Dict[str, int], False]: The mode of the file or False if the file doesn't exist.
    """
    from os import stat
    if (path.exists(filename)):
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

propSet('fsModule', 'statSync_inner', statSync_inner);
propSet('fsModule', 'readFileSync', readFileSync)
propSet('fsModule', 'existsSync', path.exists)
pm.eval("fsModule.constants = { S_IFDIR: 16384 }; true;")
pm.eval("""fsModule.statSync =
function statSync(filename)
{
  const ret = require('fs').statSync_inner(filename);
  if (ret)
    return ret;

  const err = new Error('file not found: ' + filename); 
  err.code='ENOENT'; 
  throw err;
}""");

# Read in ctx-module and invoke so that this file is the "main module" and the Python symbol require is
# now the corresponding CommonJS require() function.  We use the globalThis as the module's exports
# because PythonMonkey current segfaults when return objects. Once that is fixed, we will pass moduleIIFE
# parameters which are a python implementation of top-level require(for fs, vm - see top) and an exports
# dict to decorate.

with open(path.dirname(__file__) + "/node_modules/ctx-module/ctx-module.js", "r") as ctxModuleSource:
    moduleWrapper = pm.eval("""'use strict';
(function moduleWrapper(require, exports)
{
  exports=exports || globalThis; 
  require=require || globalThis.require;
""" + ctxModuleSource.read() + """
})
""");

# inject require and exports symbols as moduleWrapper arguments once jsObj->dict fixed so we don't have
# to use globalThis and pollute the global scope. 
moduleWrapper()

# __builtinModules should be a dict that we add built-in modules to in Python, then pass the same
# dict->jsObject in createRequire for every require we create.
pm.eval('const __builtinModules = {}; true');

def load(filename: str) -> Dict:  
    """
    Loads a python module using the importlib machinery sourcefileloader, prefills it with an exports object and returns the module.
    If the module is already loaded, returns it.

    Args:
        filename (str): The filename of the python module to load.

    Returns:
        : The loaded python module
    """

    name = path.basename(filename)
    if name not in sys.modules:
        sourceFileLoader = machinery.SourceFileLoader(name, filename)
        spec = importlib.util.spec_from_loader(sourceFileLoader.name, sourceFileLoader)
        module = importlib.util.module_from_spec(spec)
        sys.modules[name] = module
        module.exports = {}
        spec.loader.exec_module(module)
    else:
        module = sys.modules[name]
    module_exports = {}
    for key in dir(module):
        module_exports[key] = getattr(module, key)
    return module_exports 

propSet('python', 'load', load)

"""
API - createRequire
returns a require function that resolves modules relative to the filename argument. 
Conceptually the same as node:module.createRequire().

example:
  from pythonmonkey import createRequire
  require = createRequire(__file__)
  require('./my-javascript-module')
"""
createRequire: Callable = pm.eval("""(
function createRequire(filename)
{
  function loadPythonModule(module, filename)
  {
    module.exports = python.load(filename);
  }

  const module = new CtxModule(globalThis, filename, __builtinModules);
  for (let path of python.paths)
    module.paths.push(path + '/node_modules');
  module.require.extensions['.py'] = loadPythonModule;
  return module.require;
}
)""")
