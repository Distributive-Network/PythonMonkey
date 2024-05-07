/**
 * @file BufferType.hh
 * @author Tom Tang (xmader@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing ArrayBuffers
 * @date 2023-04-27
 *
 * @copyright Copyright (c) 2023,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_BufferType_
#define PythonMonkey_BufferType_

#include <jsapi.h>
#include <js/ScalarType.h>

#include <Python.h>

struct BufferType {
public:
  /**
   * @brief Construct a new BufferType object from a JS TypedArray or ArrayBuffer, as a Python [memoryview](https://docs.python.org/3.9/c-api/memoryview.html) object
   *
   * @param cx - javascript context pointer
   * @param bufObj - JS object to be coerced
   *
   * @returns PyObject* pointer to the resulting PyObject
   */
  static PyObject *getPyObject(JSContext *cx, JS::HandleObject bufObj);

  /**
   * @brief Convert a Python object that [provides the buffer interface](https://docs.python.org/3.9/c-api/typeobj.html#buffer-object-structures) to JS TypedArray.
   * The subtype (Uint8Array, Float64Array, ...) is automatically determined by the Python buffer's [format](https://docs.python.org/3.9/c-api/buffer.html#c.Py_buffer.format)
   *
   * @param cx - javascript context pointer
   * @param pyObject - the object to be converted
   */
  static JSObject *toJsTypedArray(JSContext *cx, PyObject *pyObject);

  /**
   * @returns Is the given JS object either a TypedArray or an ArrayBuffer?
   */
  static bool isSupportedJsTypes(JSObject *obj);

protected:
  static PyObject *fromJsTypedArray(JSContext *cx, JS::HandleObject typedArray);
  static PyObject *fromJsArrayBuffer(JSContext *cx, JS::HandleObject arrayBuffer);

private:
  static void _releasePyBuffer(Py_buffer *bufView);
  static void _releasePyBuffer(void *, void *bufView); // JS::BufferContentsFreeFunc callback for JS::NewExternalArrayBuffer

  static JS::Scalar::Type _getPyBufferType(Py_buffer *bufView);
  static const char *_toPyBufferFormatCode(JS::Scalar::Type subtype);

  /**
   * @brief Create a new typed array using up the given ArrayBuffer or SharedArrayBuffer for storage.
   * @see https://hg.mozilla.org/releases/mozilla-esr102/file/a03fde6/js/public/experimental/TypedData.h#l80
   * There's no SpiderMonkey API to assign the subtype at execution time
   */
  static JSObject *_newTypedArrayWithBuffer(JSContext *cx, JS::Scalar::Type subtype, JS::HandleObject arrayBuffer);
};

#endif