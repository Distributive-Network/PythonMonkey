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
#include <js/ScalarType.h>

#include <Python.h>

struct BufferType : public PyType {
public:
  BufferType(PyObject *object);

  const TYPE returnType = TYPE::BUFFER;

  /**
   * @brief Convert a Python object that [provides the buffer interface](https://docs.python.org/3.9/c-api/typeobj.html#buffer-object-structures) to JS TypedArray.
   * The subtype (Uint8Array, Float64Array, ...) is automatically determined by the Python buffer's [format](https://docs.python.org/3.9/c-api/buffer.html#c.Py_buffer.format)
   *
   * @param cx - javascript context pointer
   */
  JSObject *toJsTypedArray(JSContext *cx);
protected:
  virtual void print(std::ostream &os) const override;

  static void _releasePyBuffer(void *, void *bufView); // JS::BufferContentsFreeFunc

  static JS::Scalar::Type _getPyBufferType(Py_buffer *bufView);

  /**
   * @brief Create a new typed array using up the given ArrayBuffer or SharedArrayBuffer for storage.
   * @see https://hg.mozilla.org/releases/mozilla-esr102/file/a03fde6/js/public/experimental/TypedData.h#l80
   * There's no SpiderMonkey API to assign the subtype at execution time
   */
  static JSObject *_newTypedArrayWithBuffer(JSContext *cx, JS::Scalar::Type subtype, JS::HandleObject arrayBuffer);
};

#endif