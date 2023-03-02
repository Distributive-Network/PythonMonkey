# PythonMonkey

![Testing Suite](https://github.com/Kings-Distributed-Systems/PythonMonkey/actions/workflows/tests.yaml/badge.svg)

## About
PythonMonkey is a Mozilla [SpiderMonkey](https://firefox-source-docs.mozilla.org/js/index.html) JS engine embedded into the Python VM,
using the Python engine to provide the JS host environment.

This product is in an early stage, approximately 65% to MVP as of March 2023. It is under active development by Distributive Corp.,
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
- [underway] NodeJS+NPM-compatible CommonJS module system
- [underway] Python strings coerce to JS strings
- [done] Python intrinsics coerce to JS intrinsics
- Python dicts coerce to JS objects
- Python `require` function, returns a coerced dict of module exports
- Python functions coerce to JS function wrappers
- CommonJS module system .py loader, loads Python modules for use by JS
- JS object->Python dict coercion supports inherited-property lookup (via __getattribute__?)
- Python host environment supplies event loop, including EventEmitter, setTimeout, etc.
- Python host environment supplies XMLHttpRequest (other project?)
- Python host environment supplies basic subsets of NodeJS's fs, path, process, etc, modules; as-needed by dcp-client (other project?)
- Python TypedArrays coerce to JS TypeArrays
- JS TypedArrays coerce to Python TypeArrays

## Build Instructions
1. You will need the following installed:
    - pytest
    - cmake
    - doxygen 
    - python3-dev (python-dev)

2. Create a build folder in the root directory
    ```bash
    $ mkdir build
    ```

3. Next cd into the build directory and run `cmake ..`
    ```bash
    $ cd build
    $ cmake ..
    ```

4. Now run `cmake --build .` to compile the project
    ```bash
    $ cmake --build .
    ```

## Running tests
1. Compile the project 
2. In the build folder `cd` into the `tests` directory and run `ctest`
    ```bash
    # From the root directory we do the following (after compiling the project)
    $ cd buid/tests
    $ ctest
    ```

## Alternate Method to Running Tests (W.I.P)
1. In the root directory chmod the script `test_script.sh`:
    ```bash
    $ chmod u+x test_script.sh
    ```
2. Then simply run the script
    ```bash
    $ ./test_script.sh
    ```

Note that for this method to work you will still need to have the above requirements installed.

## Using the library

### Method 1
After compiling the project in the `build/src` folder you will find a `.so` file named `explore.so` this is the shared object file that contains the c++ -> python library.

If you wish to use the library you can simply copy the `.so` file into the directory that you wish.

### Method 2
**(W.I.P)**  
In the root directory you can run
```bash
pip3 install .

or

pip install .
```
To install this file into your current python environment. Then it is accessible anywhere on your machine.

