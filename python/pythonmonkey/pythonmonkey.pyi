"""
@see https://typing.readthedocs.io/en/latest/source/stubs.html
"""

import typing as _typing


class EvalOptions(_typing.TypedDict, total=False):
  filename: str
  lineno: int
  column: int
  mutedErrors: bool
  noScriptRval: bool
  selfHosting: bool
  strict: bool
  module: bool
  fromPythonFrame: bool

# pylint: disable=redefined-builtin


def eval(code: str, evalOpts: EvalOptions = {}, /) -> _typing.Any:
  """
  JavaScript evaluator in Python
  """


def wait() -> _typing.Awaitable[None]:
  """
  Block until all asynchronous jobs (Promise/setTimeout/etc.) finish.

  ```py
  await pm.wait()
  ```

  This is the event-loop shield that protects the loop from being prematurely terminated.
  """


def isCompilableUnit(code: str) -> bool:
  """
  Hint if a string might be compilable Javascript without actual evaluation
  """


def collect() -> None:
  """
  Calls the spidermonkey garbage collector
  """


class JSFunctionProxy():
  """
  JavaScript Function proxy
  """


class JSMethodProxy(JSFunctionProxy, object):
  """
  JavaScript Method proxy
  This constructs a callable object based on the first argument, bound to the second argument
  Useful when you wish to implement a method on a class object with JavaScript
  Example:
  import pythonmonkey as pm

  jsFunc = pm.eval("(function(value) { this.value = value})")
  class Class:
    def __init__(self):
      self.value = 0
      self.setValue = pm.JSMethodProxy(jsFunc, self) #setValue will be bound to self, so `this` will always be `self`

  myObject = Class()
  print(myObject.value) # 0
  myObject.setValue(42)
  print(myObject.value) # 42.0
  """

  def __init__(self) -> None:
    """
    PythonMonkey init function
    """


null = _typing.Annotated[
    _typing.NewType("pythonmonkey.null", object),
    "Representing the JS null type in Python using a singleton object",
]
