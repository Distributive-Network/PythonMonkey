/**
 * @file JSObjectProxy.cc
 * @author Caleb Aikens (caleb@distributive.network) & Tom Tang (xmader@distributive.network)
 * @brief JSObjectProxy is a custom C-implemented python type that derives from dict. It acts as a proxy for JSObjects from Spidermonkey, and behaves like a dict would.
 * @version 0.1
 * @date 2023-06-26
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

JSContext *GLOBAL_CX; /**< pointer to PythonMonkey's JSContext */

bool keyToId(PyObject *key, JS::MutableHandleId idp) {
  JS::RootedString idString(GLOBAL_CX);
  switch (PyUnicode_KIND(key))
  {
  case PyUnicode_1BYTE_KIND:
    idString.set(JS_NewStringCopyZ(GLOBAL_CX, (char *)PyUnicode_1BYTE_DATA(key)));
    break;
  case PyUnicode_2BYTE_KIND:
    idString.set(JS_NewUCStringCopyN(GLOBAL_CX, (char16_t *)PyUnicode_2BYTE_DATA(key), PyUnicode_GET_LENGTH(key)));
    break;
  case PyUnicode_4BYTE_KIND:
    // @TODO (Caleb Aikens) convert UCS4 to UTF16 and call JS_GetUCProperty
    break;
  }
  return JS_StringToId(GLOBAL_CX, idString, idp);
}

void JSObjectProxyMethodDefinitions::JSObjectProxy_dealloc(JSObjectProxy *self)
{
  // TODO (Caleb Aikens): intentional override of PyDict_Type's tp_dealloc. Probably results in leaking dict memory
  return;
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  JSObjectProxy *self;
  self = (JSObjectProxy *)type->tp_alloc(type, 0);
  if (!self)
  {
    Py_DECREF(self);
    return NULL;
  }
  self->jsObject.set(JS_NewObject(GLOBAL_CX, NULL));
  return (PyObject *)self;
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_init(JSObjectProxy *self, PyObject *args, PyObject *kwds)
{

  PyObject *dict = NULL;
  if (PyTuple_Size(args) == 0 || PyTuple_GetItem(args, 0) == Py_None) {
    // make fresh JSObject for proxy
    self->jsObject.set(JS_NewObject(GLOBAL_CX, NULL));
    return 0;
  }

  if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
  {
    return -1;
  }


  // make fresh JSObject for proxy
  self->jsObject.set(JS_NewObject(GLOBAL_CX, NULL));
  std::unordered_map<PyObject *, JS::RootedValue *> subValsMap;
  JSObjectProxy_init_helper(self->jsObject, dict, subValsMap);
  return 0;
}

void JSObjectProxyMethodDefinitions::JSObjectProxy_init_helper(JS::HandleObject jsObject, PyObject *dict, std::unordered_map<PyObject *, JS::RootedValue *> &subValsMap)
{
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(dict, &pos, &key, &value))
  {
    bool skip = false;
    if (!PyUnicode_Check(key))
    { // only accept string keys
      continue;
    }

    for (auto it: subValsMap)
    {
      if (it.first == value)
      { // if we've already seen this value before, just pass the same JS::Value
        JSObjectProxy_set_helper(jsObject, key, *(it.second));
        skip = true;
        break;
      }
    }

    if (skip)
    {
      continue;
    }

    JS::RootedValue *jsVal = new JS::RootedValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, value));
    subValsMap.insert({{value, jsVal}});
    JSObjectProxy_set_helper(jsObject, key, *jsVal);
  }
}

Py_ssize_t JSObjectProxyMethodDefinitions::JSObjectProxy_length(JSObjectProxy *self)
{
  JS::RootedIdVector props(GLOBAL_CX);
  if (!js::GetPropertyKeys(GLOBAL_CX, self->jsObject, JSITER_OWNONLY | JSITER_HIDDEN, &props))
  {
    // @TODO (Caleb Aikens) raise exception here
    return -1;
  }

  return props.length();
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_get(JSObjectProxy *self, PyObject *key)
{
  if (!PyUnicode_Check(key))
  {
    // @TODO (Caleb Aikens) raise exception here
    return NULL;
  }

  JS::RootedValue *value = new JS::RootedValue(GLOBAL_CX);
  JS::RootedId id(GLOBAL_CX);
  keyToId(key, &id);
  JS_GetPropertyById(GLOBAL_CX, self->jsObject, id, value);
  JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
  return pyTypeFactory(GLOBAL_CX, global, value)->getPyObject();
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_assign(JSObjectProxy *self, PyObject *key, PyObject *value)
{
  if (!PyUnicode_Check(key))
  {
    // @TODO (Caleb Aikens) raise exception here
    return -1;
  }

  if (value)
  { // we are setting a value
    JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, value));
    JSObjectProxy_set_helper(self->jsObject, key, jValue);
  }
  else
  { // we are deleting a value
    JS::RootedId id(GLOBAL_CX);
    keyToId(key, &id);
    JS::ObjectOpResult ignoredResult;
    JS_DeletePropertyById(GLOBAL_CX, self->jsObject, id, ignoredResult);
  }

  return 0;
}

void JSObjectProxyMethodDefinitions::JSObjectProxy_set_helper(JS::HandleObject jsObject, PyObject *key, JS::HandleValue value)
{
  JS::RootedId id(GLOBAL_CX);
  keyToId(key, &id);
  JS_SetPropertyById(GLOBAL_CX, jsObject, id, value);
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_richcompare(JSObjectProxy *self, PyObject *other, int op)
{
  if (op != Py_EQ && op != Py_NE) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  std::unordered_map<PyObject *, PyObject *> visited;

  bool isEqual = JSObjectProxy_richcompare_helper(self, other, visited);
  switch (op)
  {
  case Py_EQ:
    return PyBool_FromLong(isEqual);
  case Py_NE:
    return PyBool_FromLong(!isEqual);
  default:
    return NULL;
  }
}

bool JSObjectProxyMethodDefinitions::JSObjectProxy_richcompare_helper(JSObjectProxy *self, PyObject *other, std::unordered_map<PyObject *, PyObject *> &visited)
{
  if (!(Py_TYPE(other)->tp_iter) && !PySequence_Check(other) & !PyMapping_Check(other)) { // if other is not a container
    return false;
  }

  if ((visited.find((PyObject *)self) != visited.end() && visited[(PyObject *)self] == other) ||
      (visited.find((PyObject *)other) != visited.end() && visited[other] == (PyObject *)self)
  ) // if we've already compared these before, skip
  {
    return true;
  }

  visited.insert({{(PyObject *)self, other}});

  if (Py_TYPE((PyObject *)self) == Py_TYPE(other)) {
    JS::RootedValue selfVal(GLOBAL_CX, JS::ObjectValue(*self->jsObject));
    JS::RootedValue otherVal(GLOBAL_CX, JS::ObjectValue(*(*(JSObjectProxy *)other).jsObject));
    if (selfVal.asRawBits() == otherVal.asRawBits()) {
      return true;
    }
    bool *isEqual;
  }

  JS::RootedIdVector props(GLOBAL_CX);
  if (!js::GetPropertyKeys(GLOBAL_CX, self->jsObject, JSITER_OWNONLY | JSITER_HIDDEN, &props))
  {
    // @TODO (Caleb Aikens) raise exception here
    return NULL;
  }

  // iterate recursively through members of self and check for equality
  for (size_t i = 0; i < props.length(); i++)
  {
    JS::HandleId id = props[i];
    JS::RootedValue *key = new JS::RootedValue(GLOBAL_CX);
    key->setString(id.toString());

    JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
    PyObject *pyKey = pyTypeFactory(GLOBAL_CX, global, key)->getPyObject();
    PyObject *pyVal1 = PyObject_GetItem((PyObject *)self, pyKey);
    PyObject *pyVal2 = PyObject_GetItem((PyObject *)other, pyKey);
    if (!pyVal2) { // if other.key is NULL then not equal
      return false;
    }
    if (pyVal1 && Py_TYPE(pyVal1) == &JSObjectProxyType) { // if either subvalue is a JSObjectProxy, we need to pass around our visited map
      if (!JSObjectProxy_richcompare_helper((JSObjectProxy *)pyVal1, pyVal2, visited))
      {
        return false;
      }
    }
    else if (pyVal2 && Py_TYPE(pyVal2) == &JSObjectProxyType) {
      if (!JSObjectProxy_richcompare_helper((JSObjectProxy *)pyVal2, pyVal1, visited))
      {
        return false;
      }
    }
    else if (PyObject_RichCompare(pyVal1, pyVal2, Py_EQ) == Py_False) {
      return false;
    }
  }

  return true;
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_traverse(JSObjectProxy *self, visitproc visit, void *args) {
  // TODO (Caleb Aikens): intentional override of PyDict_Type's tp_traverse. Probably results in leaking dict memory
  return 0;
}
