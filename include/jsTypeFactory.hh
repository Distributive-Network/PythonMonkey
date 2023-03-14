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
 * @brief Function that takes a PyType and returns a corresponding JS::Value, doing shared memory management when necessary
 *
 * @param cx - Pointer to the JSContext
 * @param object - Pointer to the PyObject who's type and value we wish to encapsulate
 * @return JS::Value - A JS::Value corresponding to the PyType
 */
JS::Value jsTypeFactory(JSContext *cx, PyObject *object);

#endif