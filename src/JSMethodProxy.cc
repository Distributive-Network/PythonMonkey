/**
 * @file JSMethodProxy.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief JSMethodProxy is a custom C-implemented python type. It acts as a proxy for JSFunctions from Spidermonkey, and behaves like a method would, treating `self` as `this`.
 * @date 2023-11-14
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/JSMethodProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/setSpiderMonkeyException.hh"

#include <jsapi.h>

#include <Python.h>

void JSMethodProxyMethodDefinitions::JSMethodProxy_dealloc(JSMethodProxy *self)
{
  delete self->jsFunc;
  return;
}

PyObject *JSMethodProxyMethodDefinitions::JSMethodProxy_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds) {
  JSFunctionProxy *jsFunctionProxy;
  PyObject *im_self;

  if (!PyArg_ParseTuple(args, "O!O", &JSFunctionProxyType, &jsFunctionProxy, &im_self)) {
    return NULL;
  }

  JSMethodProxy *self = (JSMethodProxy *)subtype->tp_alloc(subtype, 0);
  if (self) {
    self->self = im_self;
    self->jsFunc = new JS::PersistentRootedObject(GLOBAL_CX);
    self->jsFunc->set(*(jsFunctionProxy->jsFunc));
  }

  return (PyObject *)self;
}

PyObject *JSMethodProxyMethodDefinitions::JSMethodProxy_call(PyObject *self, PyObject *args, PyObject *kwargs) {
  JSContext *cx = GLOBAL_CX;
  JS::RootedValue jsFunc(GLOBAL_CX, JS::ObjectValue(**((JSMethodProxy *)self)->jsFunc));
  JS::RootedValue selfValue(cx, jsTypeFactory(cx, ((JSMethodProxy *)self)->self));
  JS::RootedObject selfObject(cx);
  JS_ValueToObject(cx, selfValue, &selfObject);

  JS::RootedVector<JS::Value> jsArgsVector(cx);
  for (size_t i = 0; i < PyTuple_Size(args); i++) {
    JS::Value jsValue = jsTypeFactory(cx, PyTuple_GetItem(args, i));
    if (PyErr_Occurred()) { // Check if an exception has already been set in the flow of control
      return NULL; // Fail-fast
    }
    if (!jsArgsVector.append(jsValue)) {
      // out of memory
      setSpiderMonkeyException(cx);
      return NULL;
    }
  }

  JS::HandleValueArray jsArgs(jsArgsVector);
  JS::RootedValue jsReturnVal(cx);
  if (!JS_CallFunctionValue(cx, selfObject, jsFunc, jsArgs, &jsReturnVal)) {
    setSpiderMonkeyException(cx);
    return NULL;
  }

  if (PyErr_Occurred()) {
    return NULL;
  }

  return pyTypeFactory(cx, jsReturnVal);
}