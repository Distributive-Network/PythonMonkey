"""
Re-export public PythonMonkey APIs, adding type hints & documentations
"""

# Export remaining symbols
from .pythonmonkey import *

import typing as _typing
from . import pythonmonkey as _pm

def eval(code: str) -> _typing.Any:
    """
    JavaScript evaluator in Python
    """
    return _pm.eval(code)

def collect() -> None:
    """
    Calls the spidermonkey garbage collector
    """
    return _pm.collect()

def asUCS4(utf16Str: str) -> str:
    """
    Expects a python string in UTF16 encoding, and returns a new equivalent string in UCS4.
    Undefined behaviour if the string is not in UTF16.
    """
    return _pm.asUCS4(utf16Str)
