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


class WTF:
  """
  WTFPythonMonkey to use as a Python context manager (`with`-statement)

  Usage:
    ```py
    from pythonmonkey.lib.wtfpm import WTF

    with WTF():
      # the main entry point for the program utilizes PythonMonkey event-loop
      asyncio.run(pythonmonkey_main())
    ```
  """

  def __enter__(self):
    pass

  def __exit__(self, errType, errValue, traceback):
    if errType is None:  # no exception
      return
    elif issubclass(errType, KeyboardInterrupt):  # except KeyboardInterrupt:
      printTimersDebugInfo()
      return True  # exception suppressed
    else:  # other exceptions
      return False
