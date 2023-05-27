### Loader because Python is awkward
from os import path
from pathlib import Path

def include(relative, filename):
    if (path.exists(filename)): 
        fileHnd = open(filename, "r")
        __file__ = str(Path(relative, filename).resolve())
        exec(fileHnd.read())
        return locals()

import sys
sys.path.append(path.dirname(__file__) + '/..');

pmr = include(path.dirname(__file__), '../pythonmonkey_require.py');

### Actual test below
require = pmr['createRequire'](__file__)
require('./use-require/test1');

