/**
 * @file PyObjectProxyHandler.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief
 * @version 0.1
 * @date 2024-01-30
 *
 * Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/PyObjectProxyHandler.hh"

#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/StrType.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Conversions.h>
#include <js/Proxy.h>
#include <js/Symbol.h>
#include <js/friend/ErrorMessages.h>

#include <Python.h>

const char PyObjectProxyHandler::family = 0;

bool PyObjectProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const {
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  PyObject *keys = PyObject_Dir(self);
  size_t keysLength = PyList_Size(keys);

  PyObject *nonDunderKeys = PyList_New(0);
  for (size_t i = 0; i < keysLength; i++) {
    PyObject *key = PyList_GetItem(keys, i);
    if (Py_IsFalse(PyObject_CallMethod(key, "startswith", "(s)", "__"))) { // if key starts with "__", ignore it
      PyList_Append(nonDunderKeys, key);
    }
  }

  size_t length = PyList_Size(nonDunderKeys);

  return handleOwnPropertyKeys(cx, nonDunderKeys, length, props);
}

bool PyObjectProxyHandler::delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::ObjectOpResult &result) const {
  PyObject *attrName = idToKey(cx, id);
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  if (PyObject_SetAttr(self, attrName, NULL) < 0) {
    return result.failCantDelete(); // raises JS exception
  }
  return result.succeed();
}

bool PyObjectProxyHandler::has(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
  return hasOwn(cx, proxy, id, bp);
}

bool PyObjectProxyHandler::getOwnPropertyDescriptor(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
) const {
  PyObject *attrName = idToKey(cx, id);
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  PyObject *item = PyObject_GetAttr(self, attrName);
  if (!item) { // clear error, we will be returning undefined in this case
    PyErr_Clear();
  }

  return handleGetOwnPropertyDescriptor(cx, id, desc, item);
}

bool PyObjectProxyHandler::set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::HandleValue v, JS::HandleValue receiver,
  JS::ObjectOpResult &result) const {
  JS::RootedValue *rootedV = new JS::RootedValue(cx, v);
  PyObject *attrName = idToKey(cx, id);
  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  if (PyObject_SetAttr(self, attrName, pyTypeFactory(cx, global, rootedV)->getPyObject())) {
    return result.failCantSetInterposed(); // raises JS exception
  }
  return result.succeed();
}

bool PyObjectProxyHandler::enumerate(JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

bool PyObjectProxyHandler::hasOwn(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
  PyObject *attrName = idToKey(cx, id);
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  *bp = PyObject_HasAttr(self, attrName) == 1;
  return true;
}

bool PyObjectProxyHandler::getOwnEnumerablePropertyKeys(
  JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

void PyObjectProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {
  return handleFinalize(proxy);
}

bool PyObjectProxyHandler::defineProperty(JSContext *cx, JS::HandleObject proxy,
  JS::HandleId id,
  JS::Handle<JS::PropertyDescriptor> desc,
  JS::ObjectOpResult &result) const {
  // Block direct `Object.defineProperty` since we already have the `set` method
  return result.failInvalidDescriptor();
}

bool PyObjectProxyHandler::getBuiltinClass(JSContext *cx, JS::HandleObject proxy,
  js::ESClass *cls) const {
  *cls = js::ESClass::Object;
  return true;
}