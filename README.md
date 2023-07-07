# PythonMonkey

![Testing Suite](https://github.com/Kings-Distributed-Systems/PythonMonkey/actions/workflows/tests.yaml/badge.svg)

## About
PythonMonkey is a Mozilla [SpiderMonkey](https://firefox-source-docs.mozilla.org/js/index.html) JavaScript engine embedded into the Python VM,
using the Python engine to provide the JS host environment.

This product is in an early stage, approximately 80% to MVP as of July 2023. It is under active development by Distributive Corp.,
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

2. Run `poetry install`. This command automatically compiles the project and installs the project as well as all Python dependencies to the poetry virtualenv.

## Running tests
1. Compile the project 
2. From the root directory, run `poetry run pytest ./tests/python`

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

## API
These methods are exported from the pythonmonkey module.

### require(moduleIdentifier)
Return the exports of a CommonJS module identified by `moduleIdentifier`, using standrd CommonJS
semantics
 - modules are singletons and will never be loaded or evaluated more than once
 - moduleIdentifier is relative to the Python file invoking `require`
 - moduleIdentifier should not include a file extension
 - moduleIdentifiers which do not behing with ./, ../, or / are resolved by search require.path
   and module.paths.
 - Modules are evaluated immediately after loading
 - Modules are not loaded until they are required
 - The following extensions are supported:
 ** `.js` - JavaScript module; source code decorates `exports` object
 ** `.py` - Python module; source code decorates `exports` dict
 ** `.json` -- JSON module; exports are the result of parsing the JSON text in the file

### globalThis
A Python Dict which is equivalent to the globalThis object in JavaScript.

### createRequire(filename, extraPaths, isMain)
Factory function which returns a new require function
- filename: the pathname of the module that this require function could be used for
- extraPaths: [optional] a list of extra paths to search to resolve non-relative and non-absolute module identifiers
- isMain: [optional] True if the require function is being created for a main module

### runProgramModule(filename, argv, extraPaths)
Load and evaluate a program (main) module. Program modules must be written in JavaScript. Program modules are not
necessary unless the main entry point of your program is written in JavaScript.
- filename: the location of the JavaScript source code
- argv: the program's argument vector
- extraPaths: [optional] a list of extra paths to search to resolve non-relative and non-absolute module identifiers

Care should be taken to ensure that only one program module is run per JS context.

## Tricks
### Symbol injection via cross-language IIFE
You can use a JavaScript IIFE to create a scope in which you can inject Python symbols:
```python
    pm.eval("""'use strict';
(extraPaths) => {
    require.path.splice(require.path.length, 0, ...extraPaths);
    console.log('HEY:', require.path);
};
""")(extraPaths);
```

# Troubleshooting Tips

## REPL - pmjs 
A basic JavaScript shell, `pmjs`, ships with PythonMonkey. This shell can also run JavaScript programs with 

## CommonJS (require)
If you are having trouble with the CommonJS require function, set environment variable DEBUG='ctx-module*' and you can see the filenames it tries to laod.

### Extra Symbols
Loading the CommonJS subsystem, by using `require` or `createRequire` declares some extra symbols which may be helpful in debugging -
- `python.print` - the Python print function
- `python.getenv` - the Python getenv function
- `python.stdout` - an object with `read` and `write` methods, which read and write to stdout
- `python.stderr` - an object with `read` and `write` methods, which read and write to stderr
- `python.exec`   - the Python exec function
- `python.eval`   - the Python eval function
- `python.exit`   - the Python exit function
- `python.paths`  - the Python sys.paths object (currently a copy; will become an Array-like reflection)

