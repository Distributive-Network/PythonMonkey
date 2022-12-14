/**
 * @file pythonmonkey.hh
 * @author Caleb Aikens (caleb@kingsds.network)
 * @brief This file defines the pythonmonkey module, along with its various functions.
 * @version 0.1
 * @date 2022-09-06
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef PythonMonkey_Module_PythonMonkey
#define PythonMonkey_Module_PythonMonkey

#include "include/PyType.hh"

#include <jsapi.h>
#include <js/CompilationAndEvaluation.h>
#include <js/Initialization.h>

#include <Python.h>

/**
 * @brief Callback function passed to JS_SetGCCallback to handle PythonMonkey shared memory
 *
 * @param cx - Pointer to the JS Context (not used)
 * @param status - enum specifying whether the Callback triggered at the beginning or end of the GC Cycle
 * @param reason - reason for the GC Cycle
 * @param data -
 */
void handleSharedPythonMonkeyMemory(JSContext *cx, JSGCStatus status, JS::GCReason reason, void *data);

/**
 * @brief Destroys the JSContext and deletes associated memory. Called when python quits or faces a fatal exception.
 *
 */
static void cleanup();

/**
 * @brief This function is used to memoize PyTypes and GCThings that use the same backing store for their data,
 * so that the JS garbage collector doesn't collect memory still in use by Python. It does this by storing the
 * pointers in an unordered_map, with the key being the PyType pointer, and the value being a vector of GCThing
 * pointers.
 *
 * @param pyType - Pointer to the PyType to be memoized
 * @param GCThing  - Pointer to the GCThing to be memoized
 */
static void memoizePyTypeAndGCThing(PyType *pyType, JS::PersistentRootedValue *GCThing);

/**
 * @brief Function exposed by the python module that calls the spidermonkey garbage collector
 *
 * @param self - Pointer to the module object
 * @param args - Pointer to the python tuple of arguments (not used)
 * @return PyObject* - returns python None
 */
static PyObject *collect(PyObject *self, PyObject *args);

/**
 * @brief Function exposed by the python module to convert UTF16 strings to UCS4 strings
 *
 * @param self - Pointer to the module object
 * @param args - Pointer to the python tuple of arguments (expected to contain a UTF16-encoded string as the first element)
 * @return PyObject* - A new python string in UCS4 encoding
 */
static PyObject *asUCS4(PyObject *self, PyObject *args);

/**
 * @brief Function exposed by the python module for evaluating arbitrary JS code
 *
 * @param self - Pointer to the module object
 * @param args - Pointer to the python tuple of arguments (expected to contain JS program as a string as the first element)
 * @return PyObject* - The result of evaluating the JS program, coerced to a Python type, returned to the python user
 */
static PyObject *eval(PyObject *self, PyObject *args);

/**
 * @brief Initialization function for the module. Starts the JSContext, creates the global object, and sets cleanup functions
 *
 * @return PyObject* - The module object to be passed to the python user
 */
PyMODINIT_FUNC PyInit_pythonmonkey(void);

#endif