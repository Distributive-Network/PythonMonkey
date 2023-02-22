# PythonMonkey

![Testing Suite](https://github.com/Kings-Distributed-Systems/PythonMonkey/actions/workflows/tests.yaml/badge.svg)

## Build Instructions
1. You will need the following installed (which can be done automatically by running ``./setup.sh``):
    - pytest
    - cmake
    - doxygen 
    - python3-dev (python-dev)
    - graphviz
    - gcovr
    - llvm
    - rust
    - python3.9 or later
    - spidermonkey 102.2.0 or later

2. Compile pythonmonkey in ``/build`` (which can be done automatically by running ``./build_script.sh``)

## Running tests
1. Compile the project 
2. In the build folder `cd` into the `tests` directory and run `ctest`.
    ```bash
    # From the root directory we do the following (after compiling the project)
    $ cd buid/tests
    $ ctest
    ```
    Alternatively, from the root directory, run ``./test_script.sh``

## Using the library

### Method 1
After compiling the project in the `build/src` folder you will find a `.so` file named `pythonmonkey.so`. This is the shared object file that contains the pythonmonkey module.

If you wish to use the library you can simply copy the `.so` file into the directory that you wish to use python in.
```bash
# In the directory containg pythonmonkey.so
$ python
```
```py
Python 3.10.6 (main, Nov 14 2022, 16:10:14) [GCC 11.3.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> import pythonmonkey as pm
>>> hello = pm.eval("() => {return 'Hello from Spidermonkey!'}")
>>> hello()
'Hello from Spidermonkey!'
```