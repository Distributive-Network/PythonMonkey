/**
 * @file JSObjectProxy.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief JSObjectProxy is a custom C-implemented python type that derives from dict. It acts as a proxy for JSObjects from Spidermonkey, and behaves like a dict would.
 * @version 0.1
 * @date 2023-05-03
 * 
 * Copyright (c) 2023 Distributive Corp.
 * 
 */

#include "include/JSObjectProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"

#include <jsapi.h>
#include <jsfriendapi.h>

#include <Python.h>

void JSObjectProxyMethodDefinitions::JSObjectProxy_dealloc(JSObjectProxy *self) {
  delete self->jsObject;
  Py_TYPE(self)->tp_free((PyObject*) self);
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  JSObjectProxy *self;
  self = (JSObjectProxy *) type->tp_alloc(type, 0);
  if (!self) {
    Py_DECREF(self);
    return NULL;
  }
  self->jsObject = JS::RootedObject(cx);
  return (PyObject *) self;
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_init(JSObjectProxy *self, PyObject *args, PyObject *kwds) {
  PyObject *dict = NULL;
  if (!PyArg_ParseTuple(args,"O!", &PyDict_Type, dict)) {
    return -1;
  }

  // make fresh JSObject for proxy
  delete self->jsObject;
  self->jsObject = JS::RootedObject(cx);

  std::unordered_map<PyObject *, JS::RootedValue *> subValsMap;
  JSObjectProxy_init_helper(self->jsObject, dict, subValsMap);
}

void JSObjectProxyMethodDefinitions::JSObjectProxy_init_helper(JS::HandleObject jsObject, PyObject *dict, std::unordered_map<PyObject *, JS::RootedValue *>& subValsMap) {
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(dict, &pos, &key, &value)) {
    bool skip = false;
    if (!PyUnicode_Check(key)) { // only accept string keys
      continue;
    }

    for (auto it: subValsMap) {
      if (it.first == value) { // if we've already seen this value before, just pass the same JS::Value
        JSObjectProxy_set_helper(jsObject, key, *(it.second));
        skip = true;
        break;
      }
    }

    if (skip) {
      continue;
    }

    JS::RootedValue *jsVal = new JS::RootedValue(cx, jsTypeFactory(cx, value));
    subValsMap.insert({{value, jsVal}});
    JSObjectProxy_set_helper(jsObject, key, *jsVal);
  
  }
}

Py_ssize_t JSObjectProxyMethodDefinitions::JSObjectProxy_length(JSObjectProxy *self) {
  JS::RootedIdVector props(cx);
  if (!js::GetPropertyKeys(cx, self->jsObject, JSITER_OWNONLY | JSITER_HIDDEN, &props)) {
    // @TODO (Caleb Aikens) raise exception here
    return -1;
  }

  return props.length();
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_get(JSObjectProxy *self, PyObject *key) {
  if (!PyUnicode_Check(key)) {
    // @TODO (Caleb Aikens) raise exception here
    return NULL;
  }

  JS::RootedValue *value = new JS::RootedValue(cx);
  switch (PyUnicode_KIND(key)) {
    case PyUnicode_1BYTE_KIND:
      JS_GetProperty(cx, self->jsObject, (char*)PyUnicode_1BYTE_DATA(key), value);
      break;
    case PyUnicode_2BYTE_KIND:
      JS_GetUCProperty(cx, self->jsObject, (char16_t*)PyUnicode_2BYTE_DATA(key), PyUnicode_GET_LENGTH(key), value);
      break;
    case PyUnicode_4BYTE_KIND:
      // @TODO (Caleb Aikens) convert UCS4 to UTF16 and call JS_GetUCProperty
      break;
  }

  return pyTypeFactory(cx, global, value)->getPyObject();
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_assign(JSObjectProxy *self, PyObject *key, PyObject *value) {
  if (!PyUnicode_Check(key)) {
    // @TODO (Caleb Aikens) raise exception here
    return -1;
  }

  if (value) { // we are setting a value
    JS::RootedValue jValue(cx, jsTypeFactory(cx, value));
    JSObjectProxy_set_helper(self->jsObject, key, jValue);
  }
  else { // we are deleting a value
    JS::ObjectOpResult opResult;
    switch (PyUnicode_KIND(key)) {
    case PyUnicode_1BYTE_KIND:
      JS_DeleteProperty(cx, self->jsObject, (char*)PyUnicode_1BYTE_DATA(key));
      break;
    case PyUnicode_2BYTE_KIND:
      // @TODO (Caleb Aikens) make a ticket for mozilla to make an override for the below function that doesn't require an ObjectOpResult arg 
      JS_DeleteUCProperty(cx, self->jsObject, (char16_t*)PyUnicode_2BYTE_DATA(key), PyUnicode_GET_LENGTH(key), opResult);
      break;
    case PyUnicode_4BYTE_KIND:
      // @TODO (Caleb Aikens) convert UCS4 to UTF16 and call JS_SetUCProperty
      break;
    }
  }

  return 0;
}

void JSObjectProxyMethodDefinitions::JSObjectProxy_set_helper(JS::HandleObject jsObject, PyObject *key, JS::HandleValue value) {
  switch (PyUnicode_KIND(key)) {
      case PyUnicode_1BYTE_KIND:
        JS_SetProperty(cx, jsObject, (char*)PyUnicode_1BYTE_DATA(key), value);
        break;
      case PyUnicode_2BYTE_KIND:
        JS_SetUCProperty(cx, jsObject, (char16_t*)PyUnicode_2BYTE_DATA(key), PyUnicode_GET_LENGTH(key), value);
        break;
      case PyUnicode_4BYTE_KIND:
        // @TODO (Caleb Aikens) convert UCS4 to UTF16 and call JS_SetUCProperty
        break;
    }
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_richcompare(JSObjectProxy *self, PyObject *other, int op) {

  bool isEqual = true;
  

  JS::RootedIdVector props(cx);
  if (!js::GetPropertyKeys(cx, self->jsObject, JSITER_OWNONLY | JSITER_HIDDEN, &props)) {
    // @TODO (Caleb Aikens) raise exception here
    return NULL;
  }

  // iterate recursively through members of self and check for equality
  for (size_t i = 0; i < props.length(); i++) {
    JS::HandleId id = props[i];
    JS::RootedValue *key = new JS::RootedValue(cx, id.toString());
    PyObject *pyKey = pyTypeFactory(cx, global, key)->getPyObject();
    PyObject *pyVal1 = PyObject_GetItem((PyObject*)self, pyKey);
    PyObject *pyVal2 = PyObject_GetItem((PyObject*)other, pyKey);

    if (Py_IsFalse(PyObject_RichCompare(pyVal1, pyVal2, Py_EQ))) {
      isEqual = false;
      break;
    }
    // @TODO (Caleb Aikens) how do we handle self-reference?
  }

  switch (op) {
    case Py_EQ:
    return PyBool_FromLong(isEqual);
    case Py_NE:
    return PyBool_FromLong(!isEqual);
    default:
    Py_RETURN_NOTIMPLEMENTED;
  }
}

/* @TODO (Caleb Aikens)
DONE '__delitem__', # mp_ass_subscript, Deletes Key if present, raises KeyError otherwise
DONE '__doc__', # tp_doc, class documentation string
DONE '__eq__', #  tp_richcompare, returns whether this is equal to argument
DONE '__ne__', # tp_richcompare, inverse of __eq__
DONE '__getitem__', #  mp_subscript, Gets value of given Key if present, raises KeyError otherwise
DONE '__init__', # tp_init, initialization function, don't think we need this
'__iter__', # tp_iter, returns an iterator object for this dict 
DONE '__len__', # mp_length, returns number of keys
DONE '__new__', # tp_new, constructor
'__ior__', # nb_inplace_or, returns union of this and argument if argument is a dict
'__or__', # nb_or, same as above(?)
'__ror__', # same as above
'__repr__', #tp_repr, returns unambiguous string representation of this
DONE '__setitem__', # mp_ass_subscript, Sets given key and value
'__str__', # tp_str, returns human readable string representation of this

# These may already internally call the __dunder__ methods above, so dont implement unless you have to
'clear', # Deletes all key-value pairs
'copy', # Returns a copy of this
'fromkeys', # First arg is iterable specifying keys of the new dictionary, second arg is the value all of the keys will have (None by default). Creates a new dict
'get', # Returns value of given key
'items', # Returns a list containing a tuple for each key-value pair
'keys', # Returns a list of keys
'pop', # Deletes the given key
'popitem', # Deletes the most recently added key
'setdefault', # Sets the key-value if it doesn't exist, and returns the value of the given key
'update', # Takes a dict or iterable containing key-value pairs, and sets the keys of this
'values' # Returns a list of values
*/
// https://docs.python.org/3/c-api/typeobj.html