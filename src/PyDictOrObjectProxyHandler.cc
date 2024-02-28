/**
 * @file PyDictOrObjectProxyHandler.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief
 * @date 2024-02-13
 *
 * Copyright (c) 2024 Distributive Corp.
 *
 */

#include "include/PyDictOrObjectProxyHandler.hh"

#include "include/jsTypeFactory.hh"

#include <jsapi.h>
#include <Python.h>

bool PyDictOrObjectProxyHandler::object_toString(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setString(JS_NewStringCopyZ(cx, "[object Object]"));
  return true;
}

bool PyDictOrObjectProxyHandler::object_toLocaleString(JSContext *cx, unsigned argc, JS::Value *vp) {
  return object_toString(cx, argc, vp);
}

bool PyDictOrObjectProxyHandler::object_valueOf(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  // return ref to self
  args.rval().set(jsTypeFactory(cx, self));
  return true;
}

JSMethodDef PyDictOrObjectProxyHandler::object_methods[] = {
  {"toString", PyDictOrObjectProxyHandler::object_toString, 0},
  {"toLocaleString", PyDictOrObjectProxyHandler::object_toLocaleString, 0},
  {"valueOf", PyDictOrObjectProxyHandler::object_valueOf, 0},
  {NULL, NULL, 0}
};


bool PyDictOrObjectProxyHandler::handleOwnPropertyKeys(JSContext *cx, PyObject *keys, size_t length, JS::MutableHandleIdVector props) {
  if (!props.reserve(length)) {
    return false; // out of memory
  }

  for (size_t i = 0; i < length; i++) {
    PyObject *key = PyList_GetItem(keys, i);
    JS::RootedId jsId(cx);
    if (!keyToId(key, &jsId)) {
      continue; // skip over keys that are not str or int
    }
    props.infallibleAppend(jsId);
  }
  return true;
}

bool PyDictOrObjectProxyHandler::handleGetOwnPropertyDescriptor(JSContext *cx, JS::HandleId id,
  JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc, PyObject *item) {
  // see if we're calling a function
  if (id.isString()) {
    for (size_t index = 0;; index++) {
      bool isThatFunction;
      const char *methodName = object_methods[index].name;
      if (methodName == NULL) {
        break;
      }
      else if (JS_StringEqualsAscii(cx, id.toString(), methodName, &isThatFunction) && isThatFunction) {
        JSFunction *newFunction = JS_NewFunction(cx, object_methods[index].call, object_methods[index].nargs, 0, NULL);
        if (!newFunction) return false;
        JS::RootedObject funObj(cx, JS_GetFunctionObject(newFunction));
        desc.set(mozilla::Some(
          JS::PropertyDescriptor::Data(
            JS::ObjectValue(*funObj),
            {JS::PropertyAttribute::Enumerable}
          )
        ));
        return true;
      }
    }
  }

  if (!item) { // NULL if the key is not present
    desc.set(mozilla::Nothing()); // JS objects return undefined for nonpresent keys
  } else {
    desc.set(mozilla::Some(
      JS::PropertyDescriptor::Data(
        jsTypeFactory(cx, item),
        {JS::PropertyAttribute::Writable, JS::PropertyAttribute::Enumerable}
      )
    ));
  }
  return true;
}

void PyDictOrObjectProxyHandler::handleFinalize(JSObject *proxy) {
  // We cannot call Py_DECREF here when shutting down as the thread state is gone.
  // Then, when shutting down, there is only on reference left, and we don't need
  // to free the object since the entire process memory is being released.
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  if (Py_REFCNT(self) > 1) {
    Py_DECREF(self);
  }
}