/**
 * @file JSObjectKeysProxy.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSObjectKeysProxy is a custom C-implemented python type that derives from dict keys
 * @date 2024-01-18
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#include "include/JSObjectKeysProxy.hh"

#include "include/JSObjectIterProxy.hh"
#include "include/JSObjectProxy.hh"
#include "include/JSArrayProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/jsTypeFactory.hh"
#include "include/PyDictProxyHandler.hh"

#include <jsapi.h>
#include <jsfriendapi.h>

#include <Python.h>



void JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_dealloc(JSObjectKeysProxy *self)
{
  PyObject_GC_UnTrack(self);
  Py_XDECREF(self->dv.dv_dict);
  PyObject_GC_Del(self);
}

Py_ssize_t JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_length(JSObjectKeysProxy *self)
{
  if (self->dv.dv_dict == NULL) {
    return 0;
  }
  return JSObjectProxyMethodDefinitions::JSObjectProxy_length((JSObjectProxy *)self->dv.dv_dict);
}

int JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_contains(JSObjectKeysProxy *self, PyObject *key)
{
  if (self->dv.dv_dict == NULL) {
    return 0;
  }
  return JSObjectProxyMethodDefinitions::JSObjectProxy_contains((JSObjectProxy *)self->dv.dv_dict, key);
}

int JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_traverse(JSObjectKeysProxy *self, visitproc visit, void *arg) {
  Py_VISIT(self->dv.dv_dict);
  return 0;
}

int JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_clear(JSObjectKeysProxy *self) {
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
    if (PyObject_TypeCheck(other, &JSObjectKeysProxyType)) {
      JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_contains((JSObjectKeysProxy *)other, next);
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

PyObject *JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_richcompare(JSObjectKeysProxy *self, PyObject *other, int op) {
  Py_ssize_t len_self, len_other;
  int ok;
  PyObject *result;

  if (!PyAnySet_Check(other) && !PyDictViewSet_Check(other)) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  len_self = JSObjectProxyMethodDefinitions::JSObjectProxy_length((JSObjectProxy *)self->dv.dv_dict);
  if (len_self < 0) {
    return NULL;
  }

  if (PyObject_TypeCheck(other, &JSObjectKeysProxyType)) {
    len_other = JSObjectProxyMethodDefinitions::JSObjectProxy_length((JSObjectProxy *)self->dv.dv_dict);
  }
  else {
    len_other = PyObject_Size(other);
  }
  if (len_other < 0) {
    return NULL;
  }

  ok = 0;
  switch (op) {
  case Py_NE:
  case Py_EQ:
    if (len_self == len_other) {

      ok = all_contained_in((PyObject *)self, other);
    }
    if (op == Py_NE && ok >= 0) {
      ok = !ok;
    }
    break;

  case Py_LT:
    if (len_self < len_other) {
      ok = all_contained_in((PyObject *)self, other);
    }
    break;

  case Py_LE:
    if (len_self <= len_other) {
      ok = all_contained_in((PyObject *)self, other);
    }
    break;

  case Py_GT:
    if (len_self > len_other) {
      ok = all_contained_in(other, (PyObject *)self);
    }
    break;

  case Py_GE:
    if (len_self >= len_other) {
      ok = all_contained_in(other, (PyObject *)self);
    }
    break;
  }

  if (ok < 0) {
    return NULL;
  }

  result = ok ? Py_True : Py_False;

  Py_INCREF(result);
  return result;
}

PyObject *JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_iter(JSObjectKeysProxy *self) {
  JSObjectIterProxy *iterator = PyObject_GC_New(JSObjectIterProxy, &JSObjectIterProxyType);
  if (iterator == NULL) {
    return NULL;
  }
  iterator->it.reversed = false;
  iterator->it.it_index = 0;
  iterator->it.kind = KIND_KEYS;
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

PyObject *JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_iter_reverse(JSObjectKeysProxy *self) {
  JSObjectIterProxy *iterator = PyObject_GC_New(JSObjectIterProxy, &JSObjectIterProxyType);
  if (iterator == NULL) {
    return NULL;
  }
  iterator->it.reversed = true;
  iterator->it.it_index = JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_length(self) - 1;
  iterator->it.kind = KIND_KEYS;
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

PyObject *JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_repr(JSObjectKeysProxy *self) {
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

  result = PyUnicode_FromFormat("%s(%R)", PyDictKeys_Type.tp_name, seq);
  Py_DECREF(seq);

Done:
  Py_ReprLeave((PyObject *)self);
  return result;
}

// private
static Py_ssize_t dictview_len(_PyDictViewObject *dv) {
  Py_ssize_t len = 0;
  if (dv->dv_dict != NULL) {
    len = dv->dv_dict->ma_used;
  }
  return len;
}

PyObject *JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_intersect(JSObjectKeysProxy *self, PyObject *other) {
  PyObject *result;
  PyObject *it;
  PyObject *key;
  Py_ssize_t len_self;
  int rv;

  // Python interpreter swaps parameters when dict view is on right side of &
  if (!PyDictViewSet_Check(self)) {
    PyObject *tmp = other;
    other = (PyObject *)self;
    self = (JSObjectKeysProxy *)tmp;
  }

  if (PyObject_TypeCheck(self, &JSObjectKeysProxyType)) {
    len_self = JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_length(self);
  }
  else {
    len_self = dictview_len((_PyDictViewObject *)self);
  }

  // if other is a set and self is smaller than other, reuse set intersection logic
  if (PySet_Check(other) && len_self <= PyObject_Size(other)) {
    return PyObject_CallMethod(other, "intersection", "O", self);
  }

  // if other is another dict view, and it is bigger than self, swap them
  if (PyDictViewSet_Check(other)) {
    Py_ssize_t len_other = dictview_len((_PyDictViewObject *)other);
    if (len_other > len_self) {
      PyObject *tmp = other;
      other = (PyObject *)self;
      self = (JSObjectKeysProxy *)tmp;
    }
  }

  /* at this point, two things should be true
     1. self is a dictview
     2. if other is a dictview then it is smaller than self */
  result = PySet_New(NULL);
  if (result == NULL) {
    return NULL;
  }

  it = PyObject_GetIter(other);
  if (it == NULL) {
    Py_DECREF(result);
    return NULL;
  }

  while ((key = PyIter_Next(it)) != NULL) {
    if (PyObject_TypeCheck(self, &JSObjectKeysProxyType)) {
      rv = JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_contains(self, key);
    }
    else {
      if (((_PyDictViewObject *)self)->dv_dict == NULL) {
        rv = 0;
      } else {
        rv = PyDict_Contains((PyObject *)((_PyDictViewObject *)self)->dv_dict, key);
      }
    }
    if (rv < 0) {
      goto error;
    }
    if (rv) {
      if (PySet_Add(result, key)) {
        goto error;
      }
    }
    Py_DECREF(key);
  }

  Py_DECREF(it);
  if (PyErr_Occurred()) {
    Py_DECREF(result);
    return NULL;
  }
  return result;

error:
  Py_DECREF(it);
  Py_DECREF(result);
  Py_DECREF(key);
  return NULL;
}

PyObject *JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_isDisjoint(JSObjectKeysProxy *self, PyObject *other) {
  PyObject *it;
  PyObject *item = NULL;

  Py_ssize_t selfLen = JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_length(self);

  if ((PyObject *)self == other) {
    if (selfLen == 0) {
      Py_RETURN_TRUE;
    }
    else {
      Py_RETURN_FALSE;
    }
  }

  /* Iterate over the shorter object (only if other is a set,
   * because PySequence_Contains may be expensive otherwise): */
  if (PyAnySet_Check(other) || PyDictViewSet_Check(other)) {
    Py_ssize_t len_self = selfLen;
    Py_ssize_t len_other = PyObject_Size(other);
    if (len_other == -1) {
      return NULL;
    }

    if ((len_other > len_self)) {
      PyObject *tmp = other;
      other = (PyObject *)self;
      self = (JSObjectKeysProxy *)tmp;
    }
  }

  it = PyObject_GetIter(other);
  if (it == NULL) {
    return NULL;
  }

  while ((item = PyIter_Next(it)) != NULL) {
    int contains;
    if (PyObject_TypeCheck(self, &JSObjectKeysProxyType)) {
      contains = JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_contains(self, item);
    }
    else {
      contains = PySequence_Contains((PyObject *)self, item);
    }
    Py_DECREF(item);
    if (contains == -1) {
      Py_DECREF(it);
      return NULL;
    }

    if (contains) {
      Py_DECREF(it);
      Py_RETURN_FALSE;
    }
  }

  Py_DECREF(it);
  if (PyErr_Occurred()) {
    return NULL; /* PyIter_Next raised an exception. */
  }

  Py_RETURN_TRUE;
}

PyObject *JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_mapping(PyObject *self, void *Py_UNUSED(ignored)) {
  return PyDictProxy_New((PyObject *)((_PyDictViewObject *)self)->dv_dict);
}