/**
 * @file JSFunctionProxy.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief JSFunctionProxy is a custom C-implemented python type that derives from PyCFunctionObject. It acts as a proxy for JSFunctions from Spidermonkey, and behaves like a function would.
 * @version 0.1
 * @date 2023-07-05
 *
 * Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/JSFunctionProxy.hh"

#include "include/JSObjectProxy.hh"
#include "include/StrType.hh"
#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/setSpiderMonkeyException.hh"

#include <jsapi.h>
#include <js/Equality.h>
#include <js/Exception.h>
#include <js/Value.h>

#include <Python.h>

static PyMethodDef callJSFuncDef = {"JSFunctionProxy", NULL, METH_VARARGS | METH_KEYWORDS, NULL};

void JSFunctionProxyMethodDefinitions::JSFunctionProxy_dealloc(JSFunctionProxy *self)
{
  delete self->jsFunction;
  (Py_TYPE(self))->tp_base->tp_dealloc((PyObject *)self);
  return;
}

PyObject *JSFunctionProxyMethodDefinitions::JSFunctionProxy_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{
  JSFunctionProxy *self = PyObject_GC_New(JSFunctionProxy, &JSFunctionProxyType);
  if (!self) {
    return NULL;
  }
  self->func.m_ml = &callJSFuncDef;
  self->func.m_module = NULL;
  self->func.m_self = NULL;
  self->func.m_weakreflist = NULL;
  self->func.vectorcall = NULL;
  self->jsFunction = new JS::PersistentRootedObject(GLOBAL_CX);
  return (PyObject *)self;
}

int JSFunctionProxyMethodDefinitions::JSFunctionProxy_init(JSFunctionProxy *self, PyObject *args, PyObject *kwds)
{
  // make fresh JSFunction for proxy
  self->jsFunction->set(nullptr);
  return 0;
}

PyObject *JSFunctionProxyMethodDefinitions::JSFunctionProxy_get(JSFunctionProxy *self, PyObject *key)
{
  JS::RootedId id(GLOBAL_CX);
  if (!keyToId(key, &id)) {
    // TODO (Caleb Aikens): raise exception here
    return NULL; // key is not a str or int
  }

  JS::RootedValue *value = new JS::RootedValue(GLOBAL_CX);
  JS_GetPropertyById(GLOBAL_CX, *self->jsFunction, id, value);
  JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(*self->jsFunction));
  return pyTypeFactory(GLOBAL_CX, global, value)->getPyObject();
}

int JSFunctionProxyMethodDefinitions::JSFunctionProxy_assign(JSFunctionProxy *self, PyObject *key, PyObject *value)
{
  JS::RootedId id(GLOBAL_CX);
  if (!keyToId(key, &id)) { // invalid key
    // TODO (Caleb Aikens): raise exception here
    return -1;
  }

  if (value) { // we are setting a value
    JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, value));
    JS_SetPropertyById(GLOBAL_CX, *self->jsFunction, id, jValue);
  } else { // we are deleting a value
    JS::ObjectOpResult ignoredResult;
    JS_DeletePropertyById(GLOBAL_CX, *self->jsFunction, id, ignoredResult);
  }

  return 0;
}

PyObject *JSFunctionProxyMethodDefinitions::JSFunctionProxy_richcompare(JSFunctionProxy *self, PyObject *other, int op)
{
  if (op != Py_EQ && op != Py_NE) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  bool isEqual;
  if (Py_TYPE((PyObject *)self) == Py_TYPE(other)) {
    JS::RootedValue funcValSelf(GLOBAL_CX, JS::ObjectValue(**self->jsFunction));
    JS::RootedValue funcValOther(GLOBAL_CX, JS::ObjectValue(**((JSFunctionProxy *)other)->jsFunction));
    if (!JS::StrictlyEqual(GLOBAL_CX, funcValSelf, funcValOther, &isEqual)) {
      // TODO (Caleb Aikens): raise exception here
      return NULL;
    }
  }
  else {
    isEqual = false;
  }

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

PyObject *JSFunctionProxyMethodDefinitions::JSFunctionProxy_repr(JSFunctionProxy *self) {
  JS::RootedValue funcVal(GLOBAL_CX, JS::ObjectValue(**self->jsFunction));
  return StrType(GLOBAL_CX, funcVal.toString()).getPyObject();
}

PyObject *JSFunctionProxyMethodDefinitions::JSFunctionProxy_call(JSFunctionProxy *self, PyObject *args, PyObject *kwargs) {
  // @TODO (Caleb Aikens) handle kwargs
  // @TODO (Caleb Aikens) handle bidirectional exceptions
  JSContext *cx = GLOBAL_CX;

  if (PyErr_Occurred()) {
    return NULL;
  }
  if (JS_IsExceptionPending(cx)) {
    setSpiderMonkeyException(cx);
    return NULL;
  }

  JS::RootedObject *thisObj = new JS::RootedObject(cx);
  if (self->func.m_self) { // set `this` to the self of the pyfunction if it has one
    thisObj->set(&(jsTypeFactory(cx, self->func.m_self).toObject()));
  }
  else { // otherwise, default to the JS global object
    thisObj->set(JS::GetNonCCWObjectGlobal(*self->jsFunction));
  }

  JS::RootedValue *jsFunc = new JS::RootedValue(GLOBAL_CX, JS::ObjectValue(**self->jsFunction));

  JS::RootedVector<JS::Value> jsArgsVector(cx);
  for (size_t i = 0; i < PyTuple_Size(args); i++) {
    JS::Value jsValue = jsTypeFactory(cx, PyTuple_GetItem(args, i));
    if (PyErr_Occurred()) { // Check if an exception has already been set in the flow of control
      return NULL; // Fail-fast
    }
    if (!jsArgsVector.append(jsValue)) {
      setSpiderMonkeyException(cx);
      return NULL;
    }
  }

  JS::HandleValueArray jsArgs(jsArgsVector);
  JS::Rooted<JS::Value> *jsReturnVal = new JS::Rooted<JS::Value>(cx);
  if (!JS_CallFunctionValue(cx, *thisObj, *jsFunc, jsArgs, jsReturnVal)) {
    setSpiderMonkeyException(cx);
    return NULL;
  }

  return pyTypeFactory(cx, thisObj, jsReturnVal)->getPyObject();
}

int JSFunctionProxyMethodDefinitions::JSFunctionProxy_traverse(JSFunctionProxy *self, visitproc visit, void *arg) {
  Py_VISIT(self->func.m_self);
  Py_VISIT(self->func.m_module);
  return 0;
}