/**
 * @file pyTypeFactory.hh
 * @author Caleb Aikens (caleb@distributive.network), Giovanni Tedesco (giovanni@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Function for wrapping arbitrary PyObjects into the appropriate PyType class, and coercing JS types to python types
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2022, 2023, 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_PyTypeFactory_
#define PythonMonkey_PyTypeFactory_

#include <jsapi.h>

#include <Python.h>


/**
 * @brief Function that takes a JS::Value and returns a corresponding PyObject* object, doing shared memory management when necessary
 *
 * @param cx - Pointer to the javascript context of the JS::Value
 * @param rval - The JS::Value who's type and value we wish to encapsulate
 * @return PyObject* - Pointer to the object corresponding to the JS::Value
 */
PyObject *pyTypeFactory(JSContext *cx, JS::HandleValue rval);

#endif