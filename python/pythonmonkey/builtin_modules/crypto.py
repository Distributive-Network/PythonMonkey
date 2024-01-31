# @file     crypto.py
# @author   Hamada Gasmallah <hamada@distributive.network>, Will <will@distributive.network>
# @date     January 2024
import os
import pythonmonkey as pm

setRandomVal = pm.eval('''
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
setRandomVal''')

def getRandomValues(typedArr):
    # modify in place
    randomBytes = memoryview(bytearray(os.urandom(typedArr.nbytes)))
    setRandomVal(typedArr, randomBytes)
    return typedArr

# Module exports
exports['getRandomValues'] = getRandomValues # type: ignore
