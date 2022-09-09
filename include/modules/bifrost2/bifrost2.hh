/**
 * @file bifrost2.hh
 * @author Caleb Aikens (caleb@kingsds.network)
 * @brief This file defines the bifrost2 module, along with its various functions.
 * @version 0.1
 * @date 2022-09-06
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef Bifrost_Module_Bifrost2_
#define Bifrost_Module_Bifrost2_

#include <jsapi.h>
#include <js/Initialization.h>

#include <Python.h>

/**
 * @brief Destroys the JSContext and deletes associated memory. Called when python quits or faces a fatal exception.
 *
 * @author Caleb Aikens (caleb@kingsds.network)
 * @date 2022-09-09
 */
static void cleanup();

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
PyMODINIT_FUNC PyInit_bifrost2(void);

#endif