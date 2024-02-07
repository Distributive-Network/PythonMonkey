/**
 * @file PyObjectProxyHandler.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief
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
  PyObject *keys = PyObject_Dir(pyObject);
  size_t keysLength = PyList_Size(keys);

  PyObject *nonDunderKeys = PyList_New(0);
  for (size_t i = 0; i < keysLength; i++) {
    PyObject *key = PyList_GetItem(keys, i);
    PyObject *isDunder = PyObject_CallMethod(key, "startswith", "(s)", "__");
    if (Py_IsFalse(isDunder)) { // if key starts with "__", ignore it
      PyList_Append(nonDunderKeys, key);
    }
  }

  size_t length = PyList_Size(nonDunderKeys);

  if (!props.reserve(length)) {
    return false; // out of memory
  }

  for (size_t i = 0; i < length; i++) {
    PyObject *key = PyList_GetItem(nonDunderKeys, i);
    JS::RootedId jsId(cx);
    if (!keyToId(key, &jsId)) {
      // TODO (Caleb Aikens): raise exception here
      return false; // key is not a str or int
    }
    props.infallibleAppend(jsId);
  }
  return true;
}

bool PyObjectProxyHandler::delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::ObjectOpResult &result) const {
  PyObject *attrName = idToKey(cx, id);
  if (PyObject_SetAttr(pyObject, attrName, NULL) < 0) {
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
  PyObject *item = PyObject_GetAttr(pyObject, attrName);
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

bool PyObjectProxyHandler::set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::HandleValue v, JS::HandleValue receiver,
  JS::ObjectOpResult &result) const {
  JS::RootedValue rootedV(cx, v);
  PyObject *attrName = idToKey(cx, id);
  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  if (PyObject_SetAttr(pyObject, attrName, pyTypeFactory(cx, global, rootedV)->getPyObject())) {
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
  *bp = PyObject_HasAttr(pyObject, attrName) == 1;
  return true;
}

bool PyObjectProxyHandler::getOwnEnumerablePropertyKeys(
  JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

void PyObjectProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {
  // We cannot call Py_DECREF here when shutting down as the thread state is gone.
  // Then, when shutting down, there is only on reference left, and we don't need
  // to free the object since the entire process memory is being released.
  if (Py_REFCNT(pyObject) > 1) {
    Py_DECREF(pyObject);
  }
}

bool PyObjectProxyHandler::defineProperty(JSContext *cx, JS::HandleObject proxy,
  JS::HandleId id,
  JS::Handle<JS::PropertyDescriptor> desc,
  JS::ObjectOpResult &result) const {
  // Block direct `Object.defineProperty` since we already have the `set` method
  return result.failInvalidDescriptor();
}