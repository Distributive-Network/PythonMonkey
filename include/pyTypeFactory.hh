/**
 * @file pyTypeFactory.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Function for wrapping arbitrary PyObjects into the appropriate PyType class, and coercing JS types to python types
 * @version 0.1
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PythonMonkey_PyTypeFactory_
#define PythonMonkey_PyTypeFactory_

#include "PyType.hh"

#include <jsapi.h>

#include <Python.h>

/** @brief Function that takes an arbitrary PyObject* and returns a corresponding PyType* object

    @author Caleb Aikens
    @date August 2022

    @param object - Pointer to the PyObject who's type and value we wish to encapsulate
 */
PyType *pyTypeFactory(PyObject *object);

/**
 * @brief Function that takes a JS::Value and returns a corresponding PyType* object, doing shared memory management when necessary
 *
 * @param cx - Pointer to the javascript context of the JS::Value
 * @param thisObj - Pointer to the JS `this` object for the value's scope
 * @param rval - Pointer to the JS::Value who's type and value we wish to encapsulate
 * @return PyType* - Pointer to a PyType object corresponding to the JS::Value
 */
PyType *pyTypeFactory(JSContext *cx, JS::Rooted<JSObject *> *thisObj, JS::Rooted<JS::Value> *rval);

#endif