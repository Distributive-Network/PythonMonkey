"""
stub file for type hints & documentations for the native module
@see https://typing.readthedocs.io/en/latest/source/stubs.html
"""

import typing as _typing

# pylint: disable=redefined-builtin
def eval(code: str, /) -> _typing.Any:
    """
    JavaScript evaluator in Python
    """

def collect() -> None:
    """
    Calls the spidermonkey garbage collector
    """

@_typing.overload
def asUCS4(utf16_str: str, /) -> str:
    """
    Expects a python string in UTF16 encoding, and returns a new equivalent string in UCS4.
    Undefined behaviour if the string is not in UTF16.
    """
@_typing.overload
def asUCS4(anything_else: _typing.Any, /) -> _typing.NoReturn: ...

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
