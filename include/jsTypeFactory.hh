/**
 * @file jsTypeFactory.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief
 * @date 2023-02-15
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JsTypeFactory_
#define PythonMonkey_JsTypeFactory_

#include <jsapi.h>

#include <Python.h>


struct PythonExternalString : public JSExternalStringCallbacks {
public:
  /**
   * @brief Get the PyObject using the given char buffer
   *
   * @param chars - the char buffer of the PyObject
   * @return PyObject* - the PyObject string
   */
  static PyObject *getPyString(const char16_t *chars);
  static PyObject *getPyString(const JS::Latin1Char *chars);

  /**
   * @brief decrefs the underlying PyObject string when the JSString is finalized
   *
   * @param chars - The char buffer of the string
   */
  void finalize(char16_t *chars) const override;
  void finalize(JS::Latin1Char *chars) const override;

  size_t sizeOfBuffer(const char16_t *chars, mozilla::MallocSizeOf mallocSizeOf) const override;
  size_t sizeOfBuffer(const JS::Latin1Char *chars, mozilla::MallocSizeOf mallocSizeOf) const override;
};
extern PythonExternalString PythonExternalStringCallbacks;

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