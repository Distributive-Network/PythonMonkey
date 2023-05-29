### Loader because Python is awkward
from os import path
from pathlib import Path

def include(relative, filename):
    __file__ = str(Path(relative, filename).resolve())
    if (path.exists(__file__)):
        exports = {}
        exports['__file__'] = __file__
        with open(__file__, "r") as fileHnd:
            exec(fileHnd.read(), exports)
        return exports 
    else:
        raise Exception('file not found: ' + __file__)

import sys
sys.path.append(path.dirname(__file__) + '/..');

pmr = include(path.dirname(__file__), '../pythonmonkey_require.py');

### Actual test below
require = pmr['createRequire'](__file__)
require('./use-python-module');
