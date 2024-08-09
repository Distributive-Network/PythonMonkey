# @file     base64.py
# @author   Tom Tang <xmader@distributive.network>, Hamada Gasmallah <hamada@distributive.network>
# @date     July 2023
# @copyright Copyright (c) 2023 Distributive Corp.

import pythonmonkey as pm
import base64


def atob(b64):
  # Workaround for pythons 'Incorrect padding' error when base64 decoding, see: 
  # https://stackoverflow.com/questions/2941995/python-ignore-incorrect-padding-error-when-base64-decoding
  append = b"==" if isinstance(b64, bytes) else "=="
  return str(base64.standard_b64decode(b64 + append), 'latin1')


def btoa(data):
  return str(base64.standard_b64encode(bytes(data, 'latin1')), 'latin1')


# Make `atob`/`btoa` globally available
pm.eval(r"""(atob, btoa) => {
  if (!globalThis.atob) {
    globalThis.atob = atob;
  }
  if (!globalThis.btoa) {
    globalThis.btoa = btoa;
  }
}""")(atob, btoa)

# Module exports
exports['atob'] = atob  # type: ignore
exports['btoa'] = btoa  # type: ignore
