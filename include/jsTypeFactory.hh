/**
 * @file jsTypeFactory.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief
 * @version 0.1
 * @date 2023-02-15
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef PythonMonkey_JsTypeFactory_
#define PythonMonkey_JsTypeFactory_

#include "include/PyType.hh"

#include <jsapi.h>

/**
 * @brief Function that takes a PyType and returns a corresponding JS::Value, doing shared memory management when necessary
 *
 * @param cx - Pointer to the JSContext
 * @param object - Pointer to the PyObject who's type and value we wish to encapsulate
 * @return JS::Value - A JS::Value corresponding to the PyType
 */
JS::Value jsTypeFactory(JSContext *cx, PyObject *object);

#endif