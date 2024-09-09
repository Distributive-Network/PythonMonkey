/**
 * @file JSFunctionProxy.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief JSFunctionProxy is a custom C-implemented python type. It acts as a proxy for JSFunctions from Spidermonkey, and behaves like a function would.
 * @date 2023-09-28
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/JSFunctionProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/setSpiderMonkeyException.hh"

#include <jsapi.h>

#include <Python.h>

void JSFunctionProxyMethodDefinitions::JSFunctionProxy_dealloc(JSFunctionProxy *self)
{
  delete self->jsFunc;
}

PyObject *JSFunctionProxyMethodDefinitions::JSFunctionProxy_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds) {
  JSFunctionProxy *self = (JSFunctionProxy *)subtype->tp_alloc(subtype, 0);
  if (self) {
    self->jsFunc = new JS::PersistentRootedObject(GLOBAL_CX);
  }
  return (PyObject *)self;
}

PyObject *JSFunctionProxyMethodDefinitions::JSFunctionProxy_call(PyObject *self, PyObject *args, PyObject *kwargs) {
  JSContext *cx = GLOBAL_CX;
  JS::RootedValue jsFunc(GLOBAL_CX, JS::ObjectValue(**((JSFunctionProxy *)self)->jsFunc));
  JSObject *jsFuncObj = jsFunc.toObjectOrNull();
  JS::RootedObject thisObj(GLOBAL_CX, JS::CurrentGlobalOrNull(GLOBAL_CX)); // if jsFunc is not bound, assume `this` is `globalThis`

  JS::RootedVector<JS::Value> jsArgsVector(cx);
  Py_ssize_t nargs = PyTuple_Size(args);
  for (size_t i = 0; i < nargs; i++) {
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
  if (!JS_CallFunctionValue(cx, thisObj, jsFunc, jsArgs, &jsReturnVal)) {
    setSpiderMonkeyException(cx);
    return NULL;
  }

  if (PyErr_Occurred()) {
    return NULL;
  }

  return pyTypeFactory(cx, jsReturnVal);
}