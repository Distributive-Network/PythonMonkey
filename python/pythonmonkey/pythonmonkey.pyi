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

def internalBinding(namespace: str) -> JSObjectProxy:
    """
    INTERNAL USE ONLY

    See function declarations in ./builtin_modules/internal-binding.d.ts
    """

def collect() -> None:
    """
    Calls the spidermonkey garbage collector
    """

class bigint(int):
    """
    Representing JavaScript BigInt in Python
    """

class SpiderMonkeyError(Exception):
    """
    Representing a corresponding JS Error in Python
    """

class JSObjectProxy(dict):
    """
    JavaScript Object proxy dict
    """
    def __init__(self) -> None: ...

null = _typing.Annotated[
    _typing.NewType("pythonmonkey.null", object),
    "Representing the JS null type in Python using a singleton object",
]
