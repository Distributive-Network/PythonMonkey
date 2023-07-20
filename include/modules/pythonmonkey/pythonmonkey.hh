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
#include "include/JobQueue.hh"

#include <jsapi.h>
#include <js/CompilationAndEvaluation.h>
#include <js/Initialization.h>

#include <Python.h>

#define PythonMonkey_Null   PyObject_GetAttrString(PyState_FindModule(&pythonmonkey), "null")   /**< macro for pythonmonkey.null object*/
#define PythonMonkey_BigInt PyObject_GetAttrString(PyState_FindModule(&pythonmonkey), "bigint") /**< macro for pythonmonkey.bigint class object */

extern JSContext *GLOBAL_CX; /**< pointer to PythonMonkey's JSContext */
static JS::Rooted<JSObject *> *global; /**< pointer to the global object of PythonMonkey's JSContext */
static JSAutoRealm *autoRealm; /**< pointer to PythonMonkey's AutoRealm */
static JobQueue *JOB_QUEUE; /**< pointer to PythonMonkey's event-loop job queue */

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
void memoizePyTypeAndGCThing(PyType *pyType, JS::Handle<JS::Value> GCThing);

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