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


struct PythonExternalString;

/**
 * @brief Function that makes a UTF16-encoded copy of a UCS4 string
 *
 * @param chars - pointer to the UCS4-encoded string
 * @param length - length of chars in code points
 * @param outStr - UTF16-encoded out-parameter string
 * @return size_t - length of outStr (counting surrogate pairs as 2)
 */
size_t UCS4ToUTF16(const uint32_t *chars, size_t length, uint16_t *outStr);

/**
 * @brief Function that takes a PyObject and returns a corresponding JS::Value, doing shared memory management when necessary
 *
 * @param cx - Pointer to the JSContext
 * @param object - Pointer to the PyObject who's type and value we wish to encapsulate
 * @return JS::Value - A JS::Value corresponding to the PyType
 */
JS::Value jsTypeFactory(JSContext *cx, PyObject *object);
/**
 * @brief same to jsTypeFactory, but it's guaranteed that no error would be set on the Python error stack, instead
 * return JS `null` on error, and output a warning in Python-land
 */
JS::Value jsTypeFactorySafe(JSContext *cx, PyObject *object);

/**
 * @brief Helper function for jsTypeFactory to create a JSFunction* through JS_NewFunction that knows how to call a python function.
 *
 * @param cx - Pointer to the JSContext
 * @param argc - The number of arguments the JSFunction expects
 * @param vp - The return value of the JSFunction
 * @return true - Function executed successfully
 * @return false - Function did not execute successfully and an exception has been set
 */
bool callPyFunc(JSContext *cx, unsigned int argc, JS::Value *vp);
#endif