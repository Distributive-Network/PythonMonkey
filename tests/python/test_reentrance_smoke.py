# @file         reentrance-smoke.py
#               Basic smoke test which shows that JS->Python->JS->Python calls work. Failures are
#               indicated by segfaults.
# @author       Wes Garland, wes@distributive.network
# @date         June 2023

import sys
import os
import pythonmonkey as pm


def test_reentrance():
  globalThis = pm.eval("globalThis;")
  globalThis.pmEval = pm.eval
  globalThis.pyEval = eval

  abc = (pm.eval("() => { return {def: pyEval('123')} };"))()
  assert (abc['def'] == 123)
  print(pm.eval("pmEval(`pyEval(\"'test passed'\")`)"))
