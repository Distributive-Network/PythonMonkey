# @file         vm-tools.py
#               A CommonJS Module written Python which feeds some Python- and JS-VM-related methods
#               back up to JS so we can use them during tests.
# @author       Wes Garland, wes@distributive.network
# @date         Jun 2023

import sys, os
sys.path.append(os.path.dirname(__file__) + '/../../../python') # location of pythonmonkey module
import pythonmonkey as pm

exports['isCompilableUnit'] = pm.isCompilableUnit
print('MODULE LOADED')
