
from .. import pythonmonkey as pm
from os import path

def register_globals(js_file: str, *python_bindings) -> None:
  with open(path.join(path.dirname(__file__), js_file), encoding="utf-8") as f:
    js_code = f.read()
  pm.eval("""
  (internalBinding, ...pythonBindings) => {
    function defineGlobal(name, value) {
      Reflect.defineProperty(globalThis, name, { value })
    }
    """ + js_code + """
  }
  """)(pm._internalBinding, *python_bindings)
