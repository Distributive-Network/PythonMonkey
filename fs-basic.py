# @file        dcp-support.py
#              Python polyfills for OS-level functionality needed by dcp-client
#
# @author      Wes Garland, wes@distributive.network
# @date        Feb 2024

import os
import pythonmonkey as pm

def readFile(filename) -> str:
    with open(filename, "r", encoding="utf8") as fileHnd:
        return fileHnd.read()

def writeFile(filename) -> str:
    with open(filename, "w", encoding="utf8") as fileHnd:
        fileHnd.write(str)

def getMode(filename) -> int:
    return os.stat(filename).st_mode

def fileExists(filename) -> bool:
    return os.path.isfile(filename)

def dirExists(dirname) -> bool:
    return os.path.isdir(dirname)

def mkdir(dirname) -> bool:
    return os.makedirs(filename)

exports['readFile']   = readFile
exports['writeFile']  = writeFile
exports['getMode']    = getMode
exports['fileExists'] = fileExists
exports['dirExists']  = dirExists
exports['constants']  = pm.eval("""'use strict';
({
  W_OK: 2,   // todo - check python docs for symbols, win32..
  R_OK: 4,
})
""")
exports['pathResolve'] = os.path.join
exports['basename']    = os.path.basename
exports['dirname']     = os.path.dirname
exports['absPath']     = os.path.isabs
exports['joinPath']    = os.path.join
exports['pathSep']     = os.path.sep
