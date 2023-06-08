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

JSContext *cx; /**< pointer to PythonMonkey's JSContext */

void JSObjectProxyMethodDefinitions::JSObjectProxy_dealloc(JSObjectProxy *self)
{
  if (Py_TYPE(self)->tp_free) {
    Py_TYPE(self)->tp_free((PyObject *)self);
  }
  else {
    Py_TYPE(self)->tp_base->tp_free((PyObject *)self);
  }

}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  JSObjectProxy *self;
  self = (JSObjectProxy *)type->tp_base->tp_alloc(type, 0);
  if (!self)
  {
    Py_DECREF(self);
    return NULL;
  }
  self->jsObject.set(JS_NewObject(cx, NULL));
  return (PyObject *)self;
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_init(JSObjectProxy *self, PyObject *args, PyObject *kwds)
{
  if (PyDict_Type.tp_init((PyObject *)self, PyTuple_New(0), kwds) < 0) {
    return -1;
  }

  PyObject *dict = NULL;
  if (PyTuple_Size(args) == 0 || Py_IsNone(PyTuple_GetItem(args, 0))) {
    // make fresh JSObject for proxy
    self->jsObject.set(JS_NewObject(cx, NULL));
    return 0;
  }

  if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, dict))
  {
    return -1;
  }


  // make fresh JSObject for proxy
  self->jsObject.set(JS_NewObject(cx, NULL));
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

    JS::RootedValue *jsVal = new JS::RootedValue(cx, jsTypeFactory(cx, value));
    subValsMap.insert({{value, jsVal}});
    JSObjectProxy_set_helper(jsObject, key, *jsVal);
  }
}

Py_ssize_t JSObjectProxyMethodDefinitions::JSObjectProxy_length(JSObjectProxy *self)
{
  JS::RootedIdVector props(cx);
  if (!js::GetPropertyKeys(cx, self->jsObject, JSITER_OWNONLY | JSITER_HIDDEN, &props))
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

  JS::RootedValue *value = new JS::RootedValue(cx);
  switch (PyUnicode_KIND(key))
  {
  case PyUnicode_1BYTE_KIND:
    JS_GetProperty(cx, self->jsObject, (char *)PyUnicode_1BYTE_DATA(key), value);
    break;
  case PyUnicode_2BYTE_KIND:
    JS_GetUCProperty(cx, self->jsObject, (char16_t *)PyUnicode_2BYTE_DATA(key), PyUnicode_GET_LENGTH(key), value);
    break;
  case PyUnicode_4BYTE_KIND:
    // @TODO (Caleb Aikens) convert UCS4 to UTF16 and call JS_GetUCProperty
    break;
  }
  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(self->jsObject));
  return pyTypeFactory(cx, global, value)->getPyObject();
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
    JS::RootedValue jValue(cx, jsTypeFactory(cx, value));
    JSObjectProxy_set_helper(self->jsObject, key, jValue);
  }
  else
  { // we are deleting a value
    JS::ObjectOpResult opResult;
    switch (PyUnicode_KIND(key))
    {
    case PyUnicode_1BYTE_KIND:
      JS_DeleteProperty(cx, self->jsObject, (char *)PyUnicode_1BYTE_DATA(key));
      break;
    case PyUnicode_2BYTE_KIND:
      // @TODO (Caleb Aikens) make a ticket for mozilla to make an override for the below function that doesn't require an ObjectOpResult arg
      JS_DeleteUCProperty(cx, self->jsObject, (char16_t *)PyUnicode_2BYTE_DATA(key), PyUnicode_GET_LENGTH(key), opResult);
      break;
    case PyUnicode_4BYTE_KIND:
      // @TODO (Caleb Aikens) convert UCS4 to UTF16 and call JS_SetUCProperty
      break;
    }
  }

  return 0;
}

void JSObjectProxyMethodDefinitions::JSObjectProxy_set_helper(JS::HandleObject jsObject, PyObject *key, JS::HandleValue value)
{
  switch (PyUnicode_KIND(key))
  {
  case PyUnicode_1BYTE_KIND:
    JS_SetProperty(cx, jsObject, (char *)PyUnicode_1BYTE_DATA(key), value);
    break;
  case PyUnicode_2BYTE_KIND:
    JS_SetUCProperty(cx, jsObject, (char16_t *)PyUnicode_2BYTE_DATA(key), PyUnicode_GET_LENGTH(key), value);
    break;
  case PyUnicode_4BYTE_KIND:
    // @TODO (Caleb Aikens) convert UCS4 to UTF16 and call JS_SetUCProperty
    break;
  }
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
  JS::RootedIdVector props(cx);
  if (!js::GetPropertyKeys(cx, self->jsObject, JSITER_OWNONLY | JSITER_HIDDEN, &props))
  {
    // @TODO (Caleb Aikens) raise exception here
    return NULL;
  }

  // iterate recursively through members of self and check for equality
  for (size_t i = 0; i < props.length(); i++)
  {
    JS::HandleId id = props[i];
    JS::RootedValue *key = new JS::RootedValue(cx);
    key->setString(id.toString());

    JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(self->jsObject));
    PyObject *pyKey = pyTypeFactory(cx, global, key)->getPyObject();
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
    else if (Py_IsFalse(PyObject_RichCompare(pyVal1, pyVal2, Py_EQ))) {
      return false;
    }
  }

  return true;
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_traverse(JSObjectProxy *self, visitproc visit, void *arg) {
  // @TODO (Caleb Aikens) currently python segfaults on exit if there are any JSObjectProxys yet to be deleted, this needs to be fixed
  return Py_TYPE(self)->tp_base->tp_traverse((PyObject *)self, visit, arg);
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_clear(JSObjectProxy *self) {
  return Py_TYPE(self)->tp_base->tp_clear((PyObject *)self);
}