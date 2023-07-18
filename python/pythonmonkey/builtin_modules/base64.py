# @file     base64.py
# @author   Tom Tang <xmader@distributive.network>, Hamada Gasmallah <hamada@distributive.network>
# @date     July 2023

import base64

atob = lambda b64: str(base64.standard_b64decode(b64), 'latin1')
btoa = lambda data: str(base64.standard_b64encode(bytes(data, 'latin1')), 'latin1')

# Make `atob`/`btoa` globally available
import pythonmonkey as pm
pm.eval(r"""(atob, btoa) => {
  if (!globalThis.atob) {
    globalThis.atob = atob;
  }
  if (!globalThis.btoa) {
    globalThis.btoa = btoa;
  }
}""")(atob, btoa)

# Module exports
exports['atob'] = atob # type: ignore
exports['btoa'] = btoa # type: ignore
