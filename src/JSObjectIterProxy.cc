/**
 * @file JSObjectIterProxy.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSObjectIterProxy is a custom C-implemented python type that derives from list iterator
 * @date 2024-01-17
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */


#include "include/JSObjectIterProxy.hh"

#include "include/JSObjectProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include "include/pyTypeFactory.hh"

#include "include/PyDictProxyHandler.hh"

#include <jsapi.h>

#include <jsfriendapi.h>

#include <Python.h>


void JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_dealloc(JSObjectIterProxy *self)
{
  delete self->it.props;
  PyObject_GC_UnTrack(self);
  Py_XDECREF(self->it.di_dict);
  PyObject_GC_Del(self);
}

int JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_traverse(JSObjectIterProxy *self, visitproc visit, void *arg) {
  Py_VISIT(self->it.di_dict);
  return 0;
}

int JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_clear(JSObjectIterProxy *self) {
  Py_CLEAR(self->it.di_dict);
  return 0;
}

PyObject *JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_iter(JSObjectIterProxy *self) {
  Py_INCREF(&self->it);
  return (PyObject *)&self->it;
}

PyObject *JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_nextkey(JSObjectIterProxy *self) {
  PyDictObject *dict = self->it.di_dict;
  if (dict == NULL) {
    return NULL;
  }

  if (self->it.reversed) {
    if (self->it.it_index >= 0) {
      JS::HandleId id = (*(self->it.props))[(self->it.it_index)--];
      PyObject *key = idToKey(GLOBAL_CX, id);
      PyObject *value;

      if (self->it.kind != KIND_KEYS) {
        JS::RootedValue jsVal(GLOBAL_CX);
        JS_GetPropertyById(GLOBAL_CX, *(((JSObjectProxy *)(self->it.di_dict))->jsObject), id, &jsVal);
        value = pyTypeFactory(GLOBAL_CX, jsVal);
      }

      PyObject *ret;
      if (self->it.kind == KIND_ITEMS) {
        ret = PyTuple_Pack(2, key, value);
      }
      else if (self->it.kind == KIND_VALUES) {
        ret = value;
      }
      else {
        ret = key;
      }

      Py_INCREF(ret);
      if (self->it.kind != KIND_KEYS) {
        Py_DECREF(value);
      }

      return ret;
    }
  } else {
    if (self->it.it_index < JSObjectProxyMethodDefinitions::JSObjectProxy_length((JSObjectProxy *)dict)) {
      JS::HandleId id = (*(self->it.props))[(self->it.it_index)++];
      PyObject *key = idToKey(GLOBAL_CX, id);
      PyObject *value;

      if (self->it.kind != KIND_KEYS) {
        JS::RootedValue jsVal(GLOBAL_CX);
        JS_GetPropertyById(GLOBAL_CX, *(((JSObjectProxy *)(self->it.di_dict))->jsObject), id, &jsVal);
        value = pyTypeFactory(GLOBAL_CX, jsVal);
      }

      PyObject *ret;
      if (self->it.kind == KIND_ITEMS) {
        ret = PyTuple_Pack(2, key, value);
      }
      else if (self->it.kind == KIND_VALUES) {
        ret = value;
      }
      else {
        ret = key;
      }

      Py_INCREF(ret);
      if (self->it.kind != KIND_KEYS) {
        Py_DECREF(value);
      }

      return ret;
    }
  }

  self->it.di_dict = NULL;
  Py_DECREF(dict);
  return NULL;
}

PyObject *JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_len(JSObjectIterProxy *self) {
  Py_ssize_t len;
  if (self->it.di_dict) {
    len = JSObjectProxyMethodDefinitions::JSObjectProxy_length((JSObjectProxy *)self->it.di_dict) - self->it.it_index;
    if (len >= 0) {
      return PyLong_FromSsize_t(len);
    }
  }
  return PyLong_FromLong(0);
}