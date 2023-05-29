### Loader because Python is awkward
import sys
from os import path
import importlib
from pathlib import Path

def include(relative, filename):
    __file__ = str(Path(relative, filename).resolve())
    name = path.basename(__file__)
    if name in sys.modules:
        return sys.modules[name]
    sourceFileLoader = importlib.machinery.SourceFileLoader(name, __file__)
    module = sourceFileLoader.load_module(name)
    return module

import sys
sys.path.append(path.dirname(__file__) + '/..');

#pmr = include(path.dirname(__file__), '../pythonmonkey_require.py');
import pythonmonkey_require as pmr

### Actual test below
require = pmr.createRequire(__file__)
require('./use-require/test1');
print("Done")

