# PythonMonkey

![Testing Suite](https://github.com/Kings-Distributed-Systems/PythonMonkey/actions/workflows/tests.yaml/badge.svg)

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

