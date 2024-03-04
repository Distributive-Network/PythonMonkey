# @file        dcp-support.py
#              Python polyfills for OS-level functionality needed by dcp-client
#
# @author      Will Pringle, will@distributive.network
# @author      Wes Garland, wes@distributive.network
# @author      Hamada Gasmallah, hamada@distributive.network
# @date        Feb 2024

import os
import pythonmonkey as pm

# Supply globalThis.crypto.getRandomValues
def getRandomValues(typedArr):
    setRandomVal = pm.eval("""'use strict';
function setRandomVal(typedArr, bytes)
{
  for (let ind=0; ind < typedArr.length; ind++)
  {
    if (typedArr.constructor.name.includes('Big'))
      typedArr[ind] = BigInt(bytes[ind]);
    else
      typedArr[ind] = bytes[ind];
  }
}
setRandomVal""")
    randomBytes = memoryview(bytearray(os.urandom(typedArr.nbytes)))
    setRandomVal(typedArr, randomBytes)
    return typedArr

exports['getRandomValues'] = getRandomValues

