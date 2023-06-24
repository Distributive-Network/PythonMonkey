# @file         use-require.py
#               Sample code which demonstrates how to use require
# @author       Wes Garland, wes@distributive.network
# @date         Jun 2023

import sys, os
sys.path.append(os.path.dirname(__file__) + '/../python') # location of pythonmonkey module
import pythonmonkey as pm

require = pm.createRequire(__file__)
require('./use-require/test1');
print("Done")

