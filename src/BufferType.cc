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

#include "include/BufferType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <jsapi.h>
#include <js/ArrayBuffer.h>
#include <js/experimental/TypedData.h>
#include <js/ScalarType.h>

#include <Python.h>

BufferType::BufferType(PyObject *object) : PyType(object) {}

void BufferType::print(std::ostream &os) const {}

JSObject *BufferType::toJsTypedArray(JSContext *cx) {
  // Get the pyObject's underlying buffer pointer and size
  Py_buffer *view = new Py_buffer{};
  if (PyObject_GetBuffer(pyObject, view, PyBUF_WRITABLE /* C-contiguous and writable 1-dimensional array */ | PyBUF_FORMAT) < 0) {
    // The exporter (pyObject) cannot provide a contiguous 1-dimensional buffer, or
    // the buffer is immutable (read-only)
    return nullptr; // raises a PyExc_BufferError
  }

  // Create a new ExternalArrayBuffer object
  // Note: data will be copied instead of transferring the ownership when this external ArrayBuffer is "transferred" to a worker thread.
  //    see https://hg.mozilla.org/releases/mozilla-esr102/file/a03fde6/js/public/ArrayBuffer.h#l86
  JSObject *arrayBuffer = JS::NewExternalArrayBuffer(cx,
    view->len /* byteLength */, view->buf /* data pointer */,
    BufferType::_releasePyBuffer, view /* the `bufView` argument to `_releasePyBuffer` */
  );
  JS::RootedObject arrayBufferRooted(cx, arrayBuffer);

  // Determine the TypedArray's subtype (Uint8Array, Float64Array, ...)
  JS::Scalar::Type subtype = _getPyBufferType(view);
  return _newTypedArrayWithBuffer(cx, subtype, arrayBufferRooted);
}

/* static */
void BufferType::_releasePyBuffer(void *, void *bufView) {
  Py_buffer *view = (Py_buffer *)bufView;
  PyBuffer_Release(view);
  delete view;
}

/* static */
JS::Scalar::Type BufferType::_getPyBufferType(Py_buffer *bufView) {
  if (!bufView->format) { // If `format` is NULL, "B" (unsigned bytes) is assumed. https://docs.python.org/3.9/c-api/buffer.html#c.Py_buffer.format
    return JS::Scalar::Uint8;
  }
  if (std::char_traits<char>::length(bufView->format) != 1) { // the type code should be a single character
    return JS::Scalar::MaxTypedArrayViewType; // invalid
  }

  char typeCode = bufView->format[0];
  // floating point types
  if (typeCode == 'f') {
    return JS::Scalar::Float32;
  } else if (typeCode == 'd') {
    return JS::Scalar::Float64;
  }

  // integer types
  // We can't rely on the type codes alone since the typecodes are mapped to C types and would have different sizes on different architectures
  //    see https://docs.python.org/3.9/library/array.html#module-array
  //        https://github.com/python/cpython/blob/7cb3a44/Modules/arraymodule.c#L550-L570
  // TODO (Tom Tang): refactor to something like `switch (typeCode) case 'Q': return [compile-time] intType<size: sizeof(long long), signed: true>`
  bool isSigned = std::islower(typeCode); // e.g. 'b' for signed char, 'B' for unsigned char
  uint8_t byteSize = bufView->itemsize;
  switch (byteSize) {
  case 1:
    return isSigned ? JS::Scalar::Int8 : JS::Scalar::Uint8; // TODO (Tom Tang): Uint8Clamped
  case 2:
    return isSigned ? JS::Scalar::Int16 : JS::Scalar::Uint16;
  case 4:
    return isSigned ? JS::Scalar::Int32 : JS::Scalar::Uint32;
  case 8:
    return isSigned ? JS::Scalar::BigInt64 : JS::Scalar::BigUint64;
  default:
    return JS::Scalar::MaxTypedArrayViewType; // invalid byteSize
  }
}

JSObject *BufferType::_newTypedArrayWithBuffer(JSContext *cx, JS::Scalar::Type subtype, JS::HandleObject arrayBuffer) {
  switch (subtype) {
#define NEW_TYPED_ARRAY_WITH_BUFFER(ExternalType, NativeType, Name) \
case JS::Scalar::Name: \
  return JS_New ## Name ## ArrayWithBuffer(cx, arrayBuffer, 0, -1 /* use up the ArrayBuffer */);

    JS_FOR_EACH_TYPED_ARRAY(NEW_TYPED_ARRAY_WITH_BUFFER)
#undef NEW_TYPED_ARRAY_WITH_BUFFER
  default: // invalid
    PyErr_SetString(PyExc_TypeError, "Invalid Python buffer type.");
    return nullptr;
  }
}


