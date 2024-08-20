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
  //printf("JSObjectIterProxy_dealloc 1 , pointer is %p, other is %p\n", self->it.props, self->iteratorSymbol);
  if (self->it.props) {
   // printf("JSObjectIterProxy_dealloc 2\n");
    delete self->it.props;
   // printf("JSObjectIterProxy_dealloc 3\n");
    PyObject_GC_UnTrack(self);
  //  printf("JSObjectIterProxy_dealloc 4\n");
    Py_XDECREF(self->it.di_dict);
   // printf("JSObjectIterProxy_dealloc 5\n");
    PyObject_GC_Del(self);
   // printf("JSObjectIterProxy_dealloc 6\n");
  }
  else if (self->iteratorSymbol) {
   // printf("JSObjectIterProxy_dealloc 7\n");
    self->iteratorSymbol->set(nullptr);
   // printf("JSObjectIterProxy_dealloc 8\n");
    delete self->iteratorSymbol;
   // printf("JSObjectIterProxy_dealloc 9\n");
  }
}

int JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_traverse(JSObjectIterProxy *self, visitproc visit, void *arg) {
  if (self->it.props) {
    Py_VISIT(self->it.di_dict);
  }
  return 0;
}

int JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_clear(JSObjectIterProxy *self) {
  if (self->it.props) {
    Py_CLEAR(self->it.di_dict);
  }
  return 0;
}

PyObject *JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_iter(JSObjectIterProxy *self) {
  //printf("JSObjectIterProxy_iter\n");
  Py_INCREF(&self->it);
  return (PyObject *)&self->it;
}

PyObject *JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_nextkey(JSObjectIterProxy *self) {
 // printf("JSObjectIterProxy_nextkey, self is %p\n", self);

  if (self->it.props) {
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
  else {
    JS::RootedObject iteratorNextObject(GLOBAL_CX, self->iteratorSymbol->get());
    JS::RootedValue nextObject(GLOBAL_CX);

    if (!JS_CallFunctionName(GLOBAL_CX, iteratorNextObject, "next", JS::HandleValueArray::empty(), &nextObject)) {
      PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSObjectIterProxyType.tp_name);
      return nullptr;
    } 

    JS::RootedObject rootedNextObject(GLOBAL_CX, nextObject.toObjectOrNull());

    JS::RootedValue nextObjectValue(GLOBAL_CX);
   
    if (!JS_GetProperty(GLOBAL_CX, rootedNextObject, "done", &nextObjectValue)) {
      PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSObjectIterProxyType.tp_name);
      return nullptr;
    }

    if (!nextObjectValue.toBoolean()) {
      // get value
      if (!JS_GetProperty(GLOBAL_CX, rootedNextObject, "value", &nextObjectValue)) {
        PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSObjectIterProxyType.tp_name);
        return nullptr;
      }

      return pyTypeFactory(GLOBAL_CX, nextObjectValue);
    } 
    else {
      return nullptr; // done with the iteration
    }
  }
}

PyObject *JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_len(JSObjectIterProxy *self) {
 // printf("JSObjectIterProxy_len self is %p\n", self);
  if (self->it.props) {
    Py_ssize_t len;
    if (self->it.di_dict) {
      len = JSObjectProxyMethodDefinitions::JSObjectProxy_length((JSObjectProxy *)self->it.di_dict) - self->it.it_index;
      if (len >= 0) {
        return PyLong_FromSsize_t(len);
      }
    }
  }
  return PyLong_FromLong(0);
}