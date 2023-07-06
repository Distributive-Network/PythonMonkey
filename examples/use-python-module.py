# @file         use-require.py
#               Sample code which demonstrates how to use require
# @author       Wes Garland, wes@distributive.network
# @date         Jun 2023

import pythonmonkey as pm

require = pm.createRequire(__file__)
require('./use-python-module');
