
from .. import pythonmonkey as pm
from os import path
import sys

def install_globals(js_file: str, *python_bindings) -> None:
  with open(path.join(path.dirname(__file__), js_file), encoding="utf-8") as f:
    js_code = f.read()
  pm.eval("""
  (internalBinding, ...pythonBindings) => {
    internalBinding = globalThis._internalBinding
    """ + js_code + """
  }
  """)(None, *python_bindings) # FIXME (Tom Tang): `pm._internalBinding` requires object coercion

install_globals("./timers.js")
install_globals("./console.js", sys.stdout.write, sys.stderr.write)
install_globals("./base64.js")
