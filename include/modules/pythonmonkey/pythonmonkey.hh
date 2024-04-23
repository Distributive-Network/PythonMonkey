/**
 * @file pythonmonkey.hh
 * @author Caleb Aikens (caleb@kingsds.network)
 * @brief This file defines the pythonmonkey module, along with its various functions.
 * @date 2022-09-06
 *
 * @copyright Copyright (c) 2022-2024 Distributive Corp.
 *
 */
#ifndef PythonMonkey_Module_PythonMonkey
#define PythonMonkey_Module_PythonMonkey

#include "include/JobQueue.hh"

#include <jsapi.h>
#include <js/CompilationAndEvaluation.h>
#include <js/Initialization.h>

#include <Python.h>


extern JSContext *GLOBAL_CX; /**< pointer to PythonMonkey's JSContext */
extern JS::PersistentRootedObject jsFunctionRegistry; /**<// this is a FinalizationRegistry for JSFunctions that depend on Python functions. It is used to handle reference counts when the JSFunction is finalized */
static JS::Rooted<JSObject *> *global; /**< pointer to the global object of PythonMonkey's JSContext */
static JSAutoRealm *autoRealm; /**< pointer to PythonMonkey's AutoRealm */
static JobQueue *JOB_QUEUE; /**< pointer to PythonMonkey's event-loop job queue */

// Get handle on global object
PyObject *getPythonMonkeyNull();
PyObject *getPythonMonkeyBigInt();

/**
 * @brief Destroys the JSContext and deletes associated memory. Called when python quits or faces a fatal exception.
 *
 */
static void cleanup();

/**
 * @brief Function exposed by the python module that calls the spidermonkey garbage collector
 *
 * @param self - Pointer to the module object
 * @param args - Pointer to the python tuple of arguments (not used)
 * @return PyObject* - returns python None
 */
static PyObject *collect(PyObject *self, PyObject *args);

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

/**
 * @brief Array of method definitions for the pythonmonkey module
 *
 */
extern PyMethodDef PythonMonkeyMethods[];

/**
 * @brief Module definition for the pythonmonkey module
 *
 */
extern struct PyModuleDef pythonmonkey;

/**
 * @brief PyObject for spidermonkey error type
 *
 */
extern PyObject *SpiderMonkeyError;
#endif