import os
import sys
import importlib
import importlib.util

WORK_DIR = os.path.join(
  os.path.realpath(os.path.dirname(__file__)),
  "pythonmonkey"
)

def import_file(module_name: str, file_path: str):
  """
  Import a Python file from its relative path directly.

  See https://docs.python.org/3/library/importlib.html#importing-a-source-file-directly
  """
  spec = importlib.util.spec_from_file_location(module_name, file_path)
  module = importlib.util.module_from_spec(spec) # type: ignore
  sys.modules[module_name] = module
  spec.loader.exec_module(module) # type: ignore
  return module

def main():
  pmpm = import_file("pmpm", os.path.join(os.path.dirname(__file__), "./pmpm.py")) # from . import pmpm
  pmpm.main(WORK_DIR) # cd pythonmonkey && npm i

if __name__ == "__main__":
  main()
