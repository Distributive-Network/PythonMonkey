# @file     WTFPythonMonkey - A tool that detects any hanging setTimeout/setInterval timers when Ctrl-C is hit
# @author   Tom Tang <xmader@distributive.network>
# @date     April 2024
# @copyright Copyright (c) 2024 Distributive Corp.

import pythonmonkey as pm

def printTimersDebugInfo():
    pm.eval("""(require) => {
        const internalBinding = require('internal-binding');
        const { getAllRefedTimersDebugInfo: getDebugInfo } = internalBinding('timers');
        console.log(getDebugInfo())
        console.log(new Date())
    }""")(pm.createRequire(__file__))
