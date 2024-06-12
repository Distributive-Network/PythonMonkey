"""
stub file for type hints & documentations for the native module
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


def require(moduleIdentifier: str, /) -> JSObjectProxy:
  """
  Return the exports of a CommonJS module identified by `moduleIdentifier`, using standard CommonJS semantics
  """


def new(ctor: JSFunctionProxy) -> _typing.Callable[..., _typing.Any]:
  """
  Wrap the JS new operator, emitting a lambda which constructs a new
  JS object upon invocation
  """


def typeof(jsval: _typing.Any, /):
  """
  This is the JS `typeof` operator, wrapped in a function so that it can be used easily from Python.
  """


def wait() -> _typing.Awaitable[None]:
  """
  Block until all asynchronous jobs (Promise/setTimeout/etc.) finish.

  ```py
  await pm.wait()
  ```

  This is the event-loop shield that protects the loop from being prematurely terminated.
  """


def stop() -> None:
  """
  Stop all pending asynchronous jobs, and unblock `await pm.wait()`
  """


def runProgramModule(filename: str, argv: _typing.List[str], extraPaths: _typing.List[str] = []) -> None:
  """
  Load and evaluate a program (main) module. Program modules must be written in JavaScript.
  """


def isCompilableUnit(code: str) -> bool:
  """
  Hint if a string might be compilable Javascript without actual evaluation
  """


def collect() -> None:
  """
  Calls the spidermonkey garbage collector
  """


def internalBinding(namespace: str) -> JSObjectProxy:
  """
  INTERNAL USE ONLY

  See function declarations in ./builtin_modules/internal-binding.d.ts
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

  def __init__(self) -> None: "deleted"


class JSObjectProxy(dict):
  """
  JavaScript Object proxy dict
  """

  def __init__(self) -> None: "deleted"


class JSArrayProxy(list):
  """
  JavaScript Array proxy
  """

  def __init__(self) -> None: "deleted"


class JSArrayIterProxy(_typing.Iterator):
  """
  JavaScript Array Iterator proxy
  """

  def __init__(self) -> None: "deleted"


class JSStringProxy(str):
  """
  JavaScript String proxy
  """

  def __init__(self) -> None: "deleted"


class bigint(int):
  """
  Representing JavaScript BigInt in Python
  """


class SpiderMonkeyError(Exception):
  """
  Representing a corresponding JS Error in Python
  """


null = _typing.Annotated[
  _typing.NewType("pythonmonkey.null", object),
  "Representing the JS null type in Python using a singleton object",
]


globalThis = _typing.Annotated[
  JSObjectProxy,
  "A Python Dict which is equivalent to the globalThis object in JavaScript",
]
