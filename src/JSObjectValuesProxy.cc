/**
 * @file JSObjectValuesProxy.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSObjectValuesProxy is a custom C-implemented python type that derives from dict values
 * @date 2024-01-19
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#include "include/JSObjectValuesProxy.hh"

#include "include/JSObjectIterProxy.hh"
#include "include/JSObjectProxy.hh"
#include "include/JSArrayProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/jsTypeFactory.hh"
#include "include/PyDictProxyHandler.hh"

#include <jsapi.h>
#include <jsfriendapi.h>

#include <Python.h>



void JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_dealloc(JSObjectValuesProxy *self)
{
  PyObject_GC_UnTrack(self);
  Py_XDECREF(self->dv.dv_dict);
  PyObject_GC_Del(self);
}

Py_ssize_t JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_length(JSObjectValuesProxy *self)
{
  if (self->dv.dv_dict == NULL) {
    return 0;
  }
  return JSObjectProxyMethodDefinitions::JSObjectProxy_length((JSObjectProxy *)self->dv.dv_dict);
}

int JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_contains(JSObjectValuesProxy *self, PyObject *key)
{
  if (self->dv.dv_dict == NULL) {
    return 0;
  }
  return JSObjectProxyMethodDefinitions::JSObjectProxy_contains((JSObjectProxy *)self->dv.dv_dict, key);
}

int JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_traverse(JSObjectValuesProxy *self, visitproc visit, void *arg) {
  Py_VISIT(self->dv.dv_dict);
  return 0;
}

int JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_clear(JSObjectValuesProxy *self) {
  Py_CLEAR(self->dv.dv_dict);
  return 0;
}

// private
static int all_contained_in(PyObject *self, PyObject *other) {
  PyObject *iter = PyObject_GetIter(self);
  int ok = 1;

  if (iter == NULL) {
    return -1;
  }

  for (;; ) {
    PyObject *next = PyIter_Next(iter);
    if (next == NULL) {
      if (PyErr_Occurred())
        ok = -1;
      break;
    }
    if (PyObject_TypeCheck(other, &JSObjectValuesProxyType)) {
      JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_contains((JSObjectValuesProxy *)other, next);
    }
    else {
      ok = PySequence_Contains(other, next);
    }
    Py_DECREF(next);
    if (ok <= 0)
      break;
  }

  Py_DECREF(iter);
  return ok;
}

PyObject *JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_iter(JSObjectValuesProxy *self) {
  JSObjectIterProxy *iterator = PyObject_GC_New(JSObjectIterProxy, &JSObjectIterProxyType);
  if (iterator == NULL) {
    return NULL;
  }
  iterator->it.reversed = false;
  iterator->it.it_index = 0;
  iterator->it.kind = KIND_VALUES;
  Py_INCREF(self->dv.dv_dict);
  iterator->it.di_dict = self->dv.dv_dict;
  iterator->it.props = new JS::PersistentRootedIdVector(GLOBAL_CX);
  // Get **enumerable** own properties
  if (!js::GetPropertyKeys(GLOBAL_CX, *(((JSObjectProxy *)(self->dv.dv_dict))->jsObject), JSITER_OWNONLY, iterator->it.props)) {
    return NULL;
  }
  PyObject_GC_Track(iterator);
  return (PyObject *)iterator;
}

PyObject *JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_iter_reverse(JSObjectValuesProxy *self) {
  JSObjectIterProxy *iterator = PyObject_GC_New(JSObjectIterProxy, &JSObjectIterProxyType);
  if (iterator == NULL) {
    return NULL;
  }
  iterator->it.reversed = true;
  iterator->it.it_index = JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_length(self) - 1;
  iterator->it.kind = KIND_VALUES;
  Py_INCREF(self->dv.dv_dict);
  iterator->it.di_dict = self->dv.dv_dict;
  iterator->it.props = new JS::PersistentRootedIdVector(GLOBAL_CX);
  // Get **enumerable** own properties
  if (!js::GetPropertyKeys(GLOBAL_CX, *(((JSObjectProxy *)(self->dv.dv_dict))->jsObject), JSITER_OWNONLY, iterator->it.props)) {
    return NULL;
  }
  PyObject_GC_Track(iterator);
  return (PyObject *)iterator;
}

PyObject *JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_repr(JSObjectValuesProxy *self) {
  PyObject *seq;
  PyObject *result = NULL;

  Py_ssize_t rc = Py_ReprEnter((PyObject *)self);
  if (rc != 0) {
    return rc > 0 ? PyUnicode_FromString("...") : NULL;
  }

  seq = PySequence_List((PyObject *)self);
  if (seq == NULL) {
    goto Done;
  }

  result = PyUnicode_FromFormat("%s(%R)", PyDictValues_Type.tp_name, seq);
  Py_DECREF(seq);
Done:
  Py_ReprLeave((PyObject *)self);
  return result;
}

PyObject *JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_mapping(PyObject *self, void *Py_UNUSED(ignored)) {
  return PyDictProxy_New((PyObject *)((_PyDictViewObject *)self)->dv_dict);
}