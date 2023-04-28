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

#include <Python.h>

BufferType::BufferType(PyObject *object) : PyType(object) {}

void BufferType::print(std::ostream &os) const {}

JSObject *BufferType::toJsArrayBuffer(JSContext *cx) {
  // Get the pyObject's underlying buffer pointer and size
  Py_buffer *view = new Py_buffer{};
  if (PyObject_GetBuffer(pyObject, view, PyBUF_WRITABLE /* C-contiguous and writable */) < 0) {
    // The exporter (pyObject) cannot provide a C-contiguous buffer,
    // also raises a PyExc_BufferError
    return nullptr;
  }

  // Create a new ExternalArrayBuffer object
  // data is copied instead of transferring the ownership when this ArrayBuffer is "transferred" to a worker thread.
  //    see https://hg.mozilla.org/releases/mozilla-esr102/file/a03fde6/js/public/ArrayBuffer.h#l86
  return JS::NewExternalArrayBuffer(cx,
    view->len, view->buf,
    BufferType::_releasePyBuffer, view
  );
}

/* static */
void BufferType::_releasePyBuffer(void *, void *bufView) {
  Py_buffer *view = (Py_buffer *)bufView;
  PyBuffer_Release(view);
  delete view;
}