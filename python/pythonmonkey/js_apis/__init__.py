
from .. import pythonmonkey as pm
from os import path

def register_globals(js_file: str, *python_bindings) -> None:
  with open(path.join(path.dirname(__file__), js_file), encoding="utf-8") as f:
    js_code = f.read()
  pm.eval("""
  (internalBinding, ...pythonBindings) => {
    internalBinding = globalThis._internalBinding
    function defineGlobal(name, value) {
      Reflect.defineProperty(globalThis, name, { value })
    }
    """ + js_code + """
  }
  """)(None, *python_bindings) # FIXME (Tom Tang): `pm._internalBinding` requires object coercion

register_globals("./timers.js")
