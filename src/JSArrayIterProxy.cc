/**
 * @file JSArrayIterProxy.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSArrayIterProxy is a custom C-implemented python type that derives from list iterator
 * @date 2024-01-15
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */


#include "include/JSArrayIterProxy.hh"

#include "include/JSArrayProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include "include/pyTypeFactory.hh"

#include <jsapi.h>

#include <Python.h>


void JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_dealloc(JSArrayIterProxy *self)
{
  PyObject_GC_UnTrack(self);
  Py_XDECREF(self->it.it_seq);
  PyObject_GC_Del(self);
}

int JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_traverse(JSArrayIterProxy *self, visitproc visit, void *arg) {
  Py_VISIT(self->it.it_seq);
  return 0;
}

int JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_clear(JSArrayIterProxy *self) {
  Py_CLEAR(self->it.it_seq);
  return 0;
}

PyObject *JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_iter(JSArrayIterProxy *self) {
  Py_INCREF(&self->it);
  return (PyObject *)&self->it;
}

PyObject *JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_next(JSArrayIterProxy *self) {
  PyListObject *seq = self->it.it_seq;
  if (seq == NULL) {
    return NULL;
  }

  if (self->it.reversed) {
    if (self->it.it_index >= 0) {
      JS::RootedValue elementVal(GLOBAL_CX);
      JS_GetElement(GLOBAL_CX, *(((JSArrayProxy *)seq)->jsArray), self->it.it_index--, &elementVal);
      return pyTypeFactory(GLOBAL_CX, elementVal);
    }
  }
  else {
    if (self->it.it_index < JSArrayProxyMethodDefinitions::JSArrayProxy_length((JSArrayProxy *)seq)) {
      JS::RootedValue elementVal(GLOBAL_CX);
      JS_GetElement(GLOBAL_CX, *(((JSArrayProxy *)seq)->jsArray), self->it.it_index++, &elementVal);
      return pyTypeFactory(GLOBAL_CX, elementVal);
    }
  }

  self->it.it_seq = NULL;
  Py_DECREF(seq);
  return NULL;
}

PyObject *JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_len(JSArrayIterProxy *self) {
  Py_ssize_t len;
  if (self->it.it_seq) {
    len = JSArrayProxyMethodDefinitions::JSArrayProxy_length((JSArrayProxy *)self->it.it_seq) - self->it.it_index;
    if (len >= 0) {
      return PyLong_FromSsize_t(len);
    }
  }
  return PyLong_FromLong(0);
}