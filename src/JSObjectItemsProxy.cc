/**
 * @file JSObjectItemsProxy.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSObjectItemsProxy is a custom C-implemented python type that derives from dict keys
 * @date 2024-01-19
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#include "include/JSObjectItemsProxy.hh"

#include "include/JSObjectIterProxy.hh"
#include "include/JSObjectProxy.hh"
#include "include/JSArrayProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/jsTypeFactory.hh"
#include "include/PyBaseProxyHandler.hh"

#include <jsapi.h>
#include <jsfriendapi.h>

#include <Python.h>



void JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_dealloc(JSObjectItemsProxy *self)
{
  PyObject_GC_UnTrack(self);
  Py_XDECREF(self->dv.dv_dict);
  PyObject_GC_Del(self);
}

Py_ssize_t JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_length(JSObjectItemsProxy *self)
{
  if (self->dv.dv_dict == NULL) {
    return 0;
  }
  return JSObjectProxyMethodDefinitions::JSObjectProxy_length((JSObjectProxy *)self->dv.dv_dict);
}

int JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_traverse(JSObjectItemsProxy *self, visitproc visit, void *arg) {
  Py_VISIT(self->dv.dv_dict);
  return 0;
}

int JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_clear(JSObjectItemsProxy *self) {
  Py_CLEAR(self->dv.dv_dict);
  return 0;
}

PyObject *JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_iter(JSObjectItemsProxy *self) {
  JSObjectIterProxy *iterator = PyObject_GC_New(JSObjectIterProxy, &JSObjectIterProxyType);
  if (iterator == NULL) {
    return NULL;
  }
  iterator->it.reversed = false;
  iterator->it.it_index = 0;
  iterator->it.kind = KIND_ITEMS;
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

PyObject *JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_iter_reverse(JSObjectItemsProxy *self) {
  JSObjectIterProxy *iterator = PyObject_GC_New(JSObjectIterProxy, &JSObjectIterProxyType);
  if (iterator == NULL) {
    return NULL;
  }
  iterator->it.reversed = true;
  iterator->it.it_index = JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_length(self) - 1;
  iterator->it.kind = KIND_ITEMS;
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

PyObject *JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_repr(JSObjectItemsProxy *self) {
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

  result = PyUnicode_FromFormat("%s(%R)", PyDictItems_Type.tp_name, seq);
  Py_DECREF(seq);

Done:
  Py_ReprLeave((PyObject *)self);
  return result;
}

PyObject *JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_mapping(PyObject *self, void *Py_UNUSED(ignored)) {
  return PyDictProxy_New((PyObject *)((_PyDictViewObject *)self)->dv_dict);
}