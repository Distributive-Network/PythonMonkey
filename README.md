# PythonMonkey

![Testing Suite](https://github.com/Kings-Distributed-Systems/PythonMonkey/actions/workflows/tests.yaml/badge.svg)

## About
PythonMonkey is a Mozilla [SpiderMonkey](https://firefox-source-docs.mozilla.org/js/index.html) JavaScript engine embedded into the Python VM,
using the Python engine to provide the JS host environment.

This product is in an early stage, approximately 80% to MVP as of May 2023. It is under active development by Distributive Corp.,
https://distributive.network/. External contributions and feedback are welcome and encouraged.

The goal is to make writing code in either JS or Python a developer preference, with libraries commonly used in either language
available eveywhere, with no significant data exchange or transformation penalties. For example, it should be possible to use NumPy 
methods from a JS library, or to refactor a slow "hot loop" written in Python to execute in JS instead, taking advantage of 
SpiderMonkey's JIT for near-native speed, rather than writing a C-language module for Python. At Distributive, we intend to use 
this package to execute our complex `dcp-client` library, which is written in JS and enables distributed computing on the web stack.

### Data Interchange
- Strings share immutable backing stores whenever possible (when allocating engine choses UCS-2 or Latin-1 internal string representation) to keep memory consumption under control, and to make it possible to move very large strings between JS and Python library code without memory-copy overhead.
- TypedArrays to share mutable backing stores; if this is not possible we will implement a copy-on-write (CoW) solution.
- JS objects are represented by Python dicts
- JS Date objects are represented by Python datetime.datetime objects
- Intrinsics (boolean, number, null, undefined) are passed by value
- JS Functions are automatically wrapped so that they behave like Python functions, and vice-versa

### Roadmap
- [done] JS instrinsics coerce to Python intrinsics
- [done] JS strings coerce to Python strings
- JS objects coerce to Python dicts [own-properties only]
- [done] JS functions coerce to Python function wrappers
- [done] JS exceptions propagate to Python
- [done] Implement `eval()` function in Python which accepts JS code and returns JS->Python coerced values
- [done] NodeJS+NPM-compatible CommonJS module system
- [done] Python strings coerce to JS strings
- [done] Python intrinsics coerce to JS intrinsics
- [done] Python dicts coerce to JS objects
- [done] Python `require` function, returns a coerced dict of module exports
- [done] Python functions coerce to JS function wrappers
- [done] CommonJS module system .py loader, loads Python modules for use by JS
- JS object->Python dict coercion supports inherited-property lookup (via __getattribute__?)
- [done] Python host environment supplies event loop, including EventEmitter, setTimeout, etc.
- Python host environment supplies XMLHttpRequest (other project?)
- Python host environment supplies basic subsets of NodeJS's fs, path, process, etc, modules; as-needed by dcp-client (other project?)
- Python TypedArrays coerce to JS TypeArrays
- JS TypedArrays coerce to Python TypeArrays

## Build Instructions
1. You will need the following installed (which can be done automatically by running ``./setup.sh``):
    - cmake
    - doxygen 
    - graphviz
    - llvm
    - rust
    - python3.8 or later with header files (python3-dev)
    - spidermonkey 102.2.0 or later
    - npm (nodejs)
    - [Poetry](https://python-poetry.org/docs/#installation)
    - [poetry-dynamic-versioning](https://github.com/mtkennerly/poetry-dynamic-versioning)

2. Run `poetry run pip install --verbose python/pminit ./`. This command automatically compiles the project and installs the project as well as dependencies into the poetry virtualenv.

## Running tests
1. Compile the project 
2. Install development dependencies: `poetry install --no-root --only=dev`
3. From the root directory, run `poetry run pytest ./tests/python`

## Using the library

### Install from [PyPI](https://pypi.org/project/pythonmonkey/)

> PythonMonkey is not release-ready yet. Our first public release is scheduled for mid-June 2023.

```bash
$ pip install pythonmonkey
```

### Install the [nightly build](https://nightly.pythonmonkey.io/)

```bash
$ pip install -i https://nightly.pythonmonkey.io/ --pre pythonmonkey
```

### Use local version

`pythonmonkey` is available in the poetry virtualenv once you compiled the project using poetry.

```bash
$ poetry run python
```
```py
Python 3.10.6 (main, Nov 14 2022, 16:10:14) [GCC 11.3.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> import pythonmonkey as pm
>>> hello = pm.eval("() => {return 'Hello from Spidermonkey!'}")
>>> hello()
'Hello from Spidermonkey!'
```

Alternatively, you can build a `wheel` package by running `poetry build --format=wheel`, and install it by `pip install dist/*.whl`.

## Examples

* [examples/](examples/)
* https://github.com/Distributive-Network/PythonMonkey-examples
* https://github.com/Distributive-Network/PythonMonkey-Crypto-JS-Fullstack-Example

# Troubleshooting Tips

## REPL - pmjs 
A basic JavaScript shell, `pmjs`, ships with PythonMonkey.

## CommonJS (require)
If you are having trouble with the CommonJS require function, set environment variable DEBUG='ctx-module*' and you can see the filenames it tries to laod.

### Extra Symbols
Loading the CommonJS subsystem declares some extra symbols which may be helpful in debugging -
- `python.print` - the Python print function
- `python.getenv` - the Python getenv function

