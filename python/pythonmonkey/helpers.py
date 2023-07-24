# @file         helpers.py - Python->JS helpers for PythonMonkey
#               - typeof operator wrapper
#               - new operator wrapper
#
# @author       Wes Garland, wes@distributive.network
# @date         July 2023
#

from . import pythonmonkey as pm 
evalOpts = { 'filename': __file__, 'fromPythonFrame': True }

def typeof(jsval):
    """
    typeof function - wraps JS typeof operator
    """
    return pm.eval("""'use strict'; (
function pmTypeof(jsval) 
{
  return typeof jsval;
}
    )""", evalOpts)(jsval);

def new(ctor):
    """
    new function - emits function which wraps JS new operator, emitting a lambda which constructs a new
    JS object upon invocation.
    """
    if (typeof(ctor) == 'string'):
        ctor = pm.eval(ctor)

    newCtor = pm.eval("""'use strict'; (
function pmNewFactory(ctor)
{
  return function newCtor(args) {
    args = Array.from(args || []);
    return new ctor(...args);
  };
}
    )""", evalOpts)(ctor)
    return (lambda *args: newCtor(list(args)))

globalThis = pm.eval('globalThis');
# standard ECMAScript global properties defined in ECMA 262-3 §15.1 except eval as it is ~supplied in pythonmonkey.so 
standard_globals = [ "Array", "Boolean", "Date", "decodeURI", "decodeURIComponent", "encodeURI",
                     "encodeURIComponent", "Error", "EvalError", "Function", "Infinity", "isNaN",
                     "isFinite", "Math", "NaN", "Number", "Object", "parseInt", "parseFloat",
                     "RangeError", "ReferenceError", "RegExp", "String", "SyntaxError", "TypeError",
                     "undefined", "URIError" ]
# SpiderMonkey-specific globals, depending on compile-time options
spidermonkey_extra_globals = [ "escape", "unescape", "uneval", "InternalError", "Script", "XML",
                               "Namespace", "QName", "File", "Generator", "Iterator", "StopIteration" ]

# Restrict what symbols are exposed to the pythonmonkey module.
__all__ = [ "new", "typeof" ]

for name in standard_globals + spidermonkey_extra_globals:
    if (globalThis['hasOwnProperty'](name)):
        globals().update({name: globalThis[name]})
        __all__.append(name)
