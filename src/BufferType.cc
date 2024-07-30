/**
 * @file BufferType.cc
 * @author Tom Tang (xmader@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing ArrayBuffers
 * @date 2023-04-27
 *
 * @copyright Copyright (c) 2023,2024 Distributive Corp.
 *
 */

#include "include/BufferType.hh"
#include "include/PyBytesProxyHandler.hh"

#include <jsapi.h>
#include <js/ArrayBuffer.h>
#include <js/experimental/TypedData.h>
#include <js/ScalarType.h>

#include <limits.h>

// JS to Python

/* static */
const char *BufferType::_toPyBufferFormatCode(JS::Scalar::Type subtype) {
  // floating point types
  switch (subtype) {
  case JS::Scalar::Float16:
    return "e";
  case JS::Scalar::Float32:
    return "f";
  case JS::Scalar::Float64:
    return "d";
  }

  // integer types
  bool isSigned = JS::Scalar::isSignedIntType(subtype);
  uint8_t byteSize = JS::Scalar::byteSize(subtype);
  // Python `array` type codes are strictly mapped to basic C types (e.g., `int`), widths may vary on different architectures,
  // but JS TypedArray uses fixed-width integer types (e.g., `uint32_t`)
  switch (byteSize) {
  case sizeof(char):
    return isSigned ? "b" : "B";
  case sizeof(short):
    return isSigned ? "h" : "H";
  case sizeof(int):
    return isSigned ? "i" : "I";
  // case sizeof(long): // compile error: duplicate case value
  //                    // And this is usually where the bit widths on 32/64-bit systems don't agree,
  //                    //    see https://en.wikipedia.org/wiki/64-bit_computing#64-bit_data_models
  //   return isSigned ? "l" : "L";
  case sizeof(long long):
    return isSigned ? "q" : "Q";
  default: // invalid
    return "x"; // type code for pad bytes, no value
  }
}

/* static */
bool BufferType::isSupportedJsTypes(JSObject *obj) {
  return JS::IsArrayBufferObject(obj) || JS_IsTypedArrayObject(obj);
}

PyObject *BufferType::getPyObject(JSContext *cx, JS::HandleObject bufObj) {
  PyObject *pyObject;
  if (JS_IsTypedArrayObject(bufObj)) {
    pyObject = fromJsTypedArray(cx, bufObj);
  } else if (JS::IsArrayBufferObject(bufObj)) {
    pyObject = fromJsArrayBuffer(cx, bufObj);
  } else {
    // TODO (Tom Tang): Add support for JS [DataView](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView)
    PyErr_SetString(PyExc_TypeError, "`bufObj` is neither a TypedArray object nor an ArraryBuffer object.");
    pyObject = nullptr;
  }

  return pyObject;
}

/* static */
PyObject *BufferType::fromJsTypedArray(JSContext *cx, JS::HandleObject typedArray) {
  JS::Scalar::Type subtype = JS_GetArrayBufferViewType(typedArray);
  auto byteLength = JS_GetTypedArrayByteLength(typedArray);

  // Retrieve/Create the underlying ArrayBuffer object for side-effect.
  //
  // If byte length is less than `JS_MaxMovableTypedArraySize()`,
  // the ArrayBuffer object would be created lazily and the data is stored inline in the TypedArray.
  // We don't want inline data because the data pointer would be invalidated during a GC as the TypedArray object is moved.
  bool isSharedMemory;
  if (!JS_GetArrayBufferViewBuffer(cx, typedArray, &isSharedMemory)) return nullptr;

  uint8_t __destBuf[0] = {}; // we don't care about its value as it's used only if the TypedArray still having inline data
  uint8_t *data = JS_GetArrayBufferViewFixedData(typedArray, __destBuf, 0 /* making sure we don't copy inline data */);
  if (data == nullptr) { // shared memory or still having inline data
    PyErr_SetString(PyExc_TypeError, "PythonMonkey cannot coerce TypedArrays backed by shared memory.");
    return nullptr;
  }

  Py_buffer bufInfo = {
    .buf = data,
    .obj = NULL /* the exporter PyObject */,
    .len = (Py_ssize_t)byteLength,
    .itemsize = (uint8_t)JS::Scalar::byteSize(subtype),
    .readonly = false,
    .ndim = 1 /* 1-dimensional array */,
    .format = (char *)_toPyBufferFormatCode(subtype),
  };
  return PyMemoryView_FromBuffer(&bufInfo);
}

/* static */
PyObject *BufferType::fromJsArrayBuffer(JSContext *cx, JS::HandleObject arrayBuffer) {
  auto byteLength = JS::GetArrayBufferByteLength(arrayBuffer);

  // TODO (Tom Tang): handle SharedArrayBuffers or disallow them completely
  bool isSharedMemory; // `JS::GetArrayBufferData` always sets this to `false`
  JS::AutoCheckCannotGC autoNoGC(cx); // we don't really care about this
  uint8_t *data = JS::GetArrayBufferData(arrayBuffer, &isSharedMemory, autoNoGC);

  Py_buffer bufInfo = {
    .buf = data,
    .obj = NULL /* the exporter PyObject */,
    .len = (Py_ssize_t)byteLength,
    .itemsize = 1 /* each element is 1 byte */,
    .readonly = false,
    .ndim = 1 /* 1-dimensional array */,
    .format = (char *)"B" /* uint8 array */,
  };
  return PyMemoryView_FromBuffer(&bufInfo);
}


// Python to JS

static PyBytesProxyHandler pyBytesProxyHandler;


JSObject *BufferType::toJsTypedArray(JSContext *cx, PyObject *pyObject) {
  Py_INCREF(pyObject);

  // Get the pyObject's underlying buffer pointer and size
  Py_buffer *view = new Py_buffer{};
  bool immutable = false;
  if (PyObject_GetBuffer(pyObject, view, PyBUF_ND | PyBUF_WRITABLE /* C-contiguous and writable */ | PyBUF_FORMAT) < 0) {
    // the buffer is immutable (e.g., Python `bytes` type is read-only)
    PyErr_Clear();     // a PyExc_BufferError was raised

    if (PyObject_GetBuffer(pyObject, view, PyBUF_ND /* C-contiguous */ | PyBUF_FORMAT) < 0) {
      return nullptr;  // a PyExc_BufferError was raised again
    }

    immutable = true;
  }
  
  if (view->ndim != 1) {
    PyErr_SetString(PyExc_BufferError, "multidimensional arrays are not allowed");
    BufferType::_releasePyBuffer(view);
    return nullptr;
  }

  // Determine the TypedArray's subtype (Uint8Array, Float64Array, ...)
  JS::Scalar::Type subtype = _getPyBufferType(view);

  JSObject *arrayBuffer;
  if (view->len > 0) {
    // Create a new ExternalArrayBuffer object
    // Note: data will be copied instead of transferring the ownership when this external ArrayBuffer is "transferred" to a worker thread.
    //    see https://hg.mozilla.org/releases/mozilla-esr102/file/a03fde6/js/public/ArrayBuffer.h#l86
    mozilla::UniquePtr<void, JS::BufferContentsDeleter> dataPtr(
      view->buf /* data pointer */,
      {BufferType::_releasePyBuffer, view /* the `bufView` argument to `_releasePyBuffer` */}
    );

    arrayBuffer = JS::NewExternalArrayBuffer(cx,
      view->len /* byteLength */, std::move(dataPtr)
    );
  } else { // empty buffer
    arrayBuffer = JS::NewArrayBuffer(cx, 0);
    BufferType::_releasePyBuffer(view); // the buffer is no longer needed since we are creating a brand new empty ArrayBuffer
  }

  if (!immutable) {
    JS::RootedObject arrayBufferRooted(cx, arrayBuffer);
    return _newTypedArrayWithBuffer(cx, subtype, arrayBufferRooted);
  } else {
    JS::RootedValue v(cx);
    JS::RootedObject uint8ArrayPrototype(cx);
    JS_GetClassPrototype(cx, JSProto_Uint8Array, &uint8ArrayPrototype); // so that instanceof will work, not that prototype methods will
    JSObject *proxy = js::NewProxyObject(cx, &pyBytesProxyHandler, v, uint8ArrayPrototype.get());
    JS::SetReservedSlot(proxy, PyObjectSlot, JS::PrivateValue(pyObject));
    JS::PersistentRootedObject *arrayBufferPointer = new JS::PersistentRootedObject(cx);
    arrayBufferPointer->set(arrayBuffer);
    JS::SetReservedSlot(proxy, OtherSlot, JS::PrivateValue(arrayBufferPointer));
    return proxy;
  }
}

/* static */
void BufferType::_releasePyBuffer(Py_buffer *bufView) {
  PyBuffer_Release(bufView);
  delete bufView;
}

/* static */
void BufferType::_releasePyBuffer(void *, void *bufView) {
  return _releasePyBuffer((Py_buffer *)bufView);
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
  } else if (typeCode == 'e') {
    return JS::Scalar::Float16;
  }


  // integer types
  // We can't rely on the type codes alone since the typecodes are mapped to C types and would have different sizes on different architectures
  //    see https://docs.python.org/3.9/library/array.html#module-array
  //        https://github.com/python/cpython/blob/7cb3a44/Modules/arraymodule.c#L550-L570
  // TODO (Tom Tang): refactor to something like `switch (typeCode) case 'Q': return [compile-time] intType<size: sizeof(long long), signed: true>`
  bool isSigned = std::islower(typeCode); // e.g. 'b' for signed char, 'B' for unsigned char
  uint8_t byteSize = bufView->itemsize;
  switch (byteSize) {
  case sizeof(int8_t):
    return isSigned ? JS::Scalar::Int8 : JS::Scalar::Uint8; // TODO (Tom Tang): Uint8Clamped
  case sizeof(int16_t):
    return isSigned ? JS::Scalar::Int16 : JS::Scalar::Uint16;
  case sizeof(int32_t):
    return isSigned ? JS::Scalar::Int32 : JS::Scalar::Uint32;
  case sizeof(int64_t):
    return isSigned ? JS::Scalar::BigInt64 : JS::Scalar::BigUint64;
  default:
    return JS::Scalar::MaxTypedArrayViewType; // invalid byteSize
  }
}

JSObject *BufferType::_newTypedArrayWithBuffer(JSContext *cx, JS::Scalar::Type subtype, JS::HandleObject arrayBuffer) {
  switch (subtype) {
#define NEW_TYPED_ARRAY_WITH_BUFFER(ExternalType, NativeType, Name) \
        case JS::Scalar::Name: \
          return JS_New ## Name ## ArrayWithBuffer(cx, arrayBuffer, 0 /* byteOffset */, -1 /* use up the ArrayBuffer */);

  JS_FOR_EACH_TYPED_ARRAY(NEW_TYPED_ARRAY_WITH_BUFFER)
#undef NEW_TYPED_ARRAY_WITH_BUFFER
  default: // invalid
    PyErr_SetString(PyExc_TypeError, "Invalid Python buffer type.");
    return nullptr;
  }
}