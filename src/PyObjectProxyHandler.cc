/**
 * @file PyObjectProxyHandler.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for creating JS proxy objects for all objects
 * @date 2024-01-30
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/PyObjectProxyHandler.hh"

#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Conversions.h>
#include <js/Proxy.h>
#include <js/Symbol.h>
#include <js/friend/ErrorMessages.h>

#include <Python.h>

const char PyObjectProxyHandler::family = 0;

bool PyObjectProxyHandler::object_toString(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setString(JS_NewStringCopyZ(cx, "[object Object]"));
  return true;
}

bool PyObjectProxyHandler::object_toLocaleString(JSContext *cx, unsigned argc, JS::Value *vp) {
  return object_toString(cx, argc, vp);
}

bool PyObjectProxyHandler::object_valueOf(JSContext *cx, unsigned argc, JS::Value *vp) {
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

JSMethodDef PyObjectProxyHandler::object_methods[] = {
  {"toString", PyObjectProxyHandler::object_toString, 0},
  {"toLocaleString", PyObjectProxyHandler::object_toLocaleString, 0},
  {"valueOf", PyObjectProxyHandler::object_valueOf, 0},
  {NULL, NULL, 0}
};

bool PyObjectProxyHandler::handleOwnPropertyKeys(JSContext *cx, PyObject *keys, size_t length, JS::MutableHandleIdVector props) {
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

bool PyObjectProxyHandler::handleGetOwnPropertyDescriptor(JSContext *cx, JS::HandleId id,
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

void PyObjectProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {
  // We cannot call Py_DECREF here when shutting down as the thread state is gone.
  // Then, when shutting down, there is only on reference left, and we don't need
  // to free the object since the entire process memory is being released.
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  if (Py_REFCNT(self) > 1) {
    Py_DECREF(self);
  }
}

bool PyObjectProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const {
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  PyObject *keys = PyObject_Dir(self);
  size_t keysLength = PyList_Size(keys);

  PyObject *nonDunderKeys = PyList_New(0);
  for (size_t i = 0; i < keysLength; i++) {
    PyObject *key = PyList_GetItem(keys, i);
    if (PyObject_CallMethod(key, "startswith", "(s)", "__") == Py_False) { // if key starts with "__", ignore it
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
  JS::RootedValue rootedV(cx, v);
  PyObject *attrName = idToKey(cx, id);

  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  PyObject *value = pyTypeFactory(cx, rootedV);
  if (PyObject_SetAttr(self, attrName, value)) {
    Py_DECREF(value);
    return result.failCantSetInterposed(); // raises JS exception
  }
  Py_DECREF(value);
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