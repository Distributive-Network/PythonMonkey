/**
 * @file BufferType.hh
 * @author Tom Tang (xmader@distributive.network)
 * @brief Struct for representing ArrayBuffers
 * @version 0.1
 * @date 2023-04-27
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_BufferType_
#define PythonMonkey_BufferType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <jsapi.h>

#include <Python.h>

struct BufferType : public PyType {
public:
  BufferType(PyObject *object);

  const TYPE returnType = TYPE::BUFFER;

  /**
   * @brief Convert a Python object that [provides the buffer interface](https://docs.python.org/3.9/c-api/typeobj.html#buffer-object-structures) to JS ArrayBuffer
   *
   * @param cx - javascript context pointer
   */
  JSObject *toJsArrayBuffer(JSContext *cx);
protected:
  virtual void print(std::ostream &os) const override;

  static void _releasePyBuffer(void *, void *bufView); // JS::BufferContentsFreeFunc
};

#endif