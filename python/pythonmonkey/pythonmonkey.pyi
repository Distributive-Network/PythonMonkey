"""
stub file for type hints & documentations for the native module
@see https://typing.readthedocs.io/en/latest/source/stubs.html
"""

import typing as _typing

@_typing.overload
def eval(code: str) -> _typing.Any:
    """
    JavaScript evaluator in Python
    """

@_typing.overload
def collect() -> None:
    """
    Calls the spidermonkey garbage collector
    """

@_typing.overload
def asUCS4(utf16_str: str) -> str:
    """
    Expects a python string in UTF16 encoding, and returns a new equivalent string in UCS4.
    Undefined behaviour if the string is not in UTF16.
    """

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
