/**
 * @file PyDictProxyHandler.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS proxy objects. Used by DictType for object coercion TODO
 * @date 2023-04-20
 *
 * Copyright (c) 2023-2024 Distributive Corp.
 *
 */

#include "include/PyDictProxyHandler.hh"

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



const char PyDictProxyHandler::family = 0;


static bool object_toString(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  args.rval().setString(JS_NewStringCopyZ(cx, "[object Object]"));
  return true;
}

static bool object_toLocaleString(JSContext *cx, unsigned argc, JS::Value *vp) {
  return object_toString(cx, argc, vp);
}

static bool object_valueOf(JSContext *cx, unsigned argc, JS::Value *vp) {
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


static JSMethodDef object_methods[] = {
  {"toString", object_toString, 0},
  {"toLocaleString", object_toLocaleString, 0},
  {"valueOf", object_valueOf, 0},
  {NULL, NULL, 0}
};


bool PyDictProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const {
  PyObject *keys = PyDict_Keys(pyObject);
  size_t length = PyList_Size(keys);
  if (!props.reserve(length)) {
    return false; // out of memory
  }

  for (size_t i = 0; i < length; i++) {
    PyObject *key = PyList_GetItem(keys, i);
    JS::RootedId jsId(cx);
    if (!keyToId(key, &jsId)) {
      // TODO (Caleb Aikens): raise exception here
      return false; // key is not a str or int
    }
    props.infallibleAppend(jsId);
  }
  return true;
}

bool PyDictProxyHandler::delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::ObjectOpResult &result) const {
  PyObject *attrName = idToKey(cx, id);
  if (PyDict_DelItem(pyObject, attrName) < 0) {
    return result.failCantDelete(); // raises JS exception
  }
  return result.succeed();
}

bool PyDictProxyHandler::has(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
  return hasOwn(cx, proxy, id, bp);
}

bool PyDictProxyHandler::getOwnPropertyDescriptor(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
) const {
  // see if we're calling a function
  if (id.isString()) {
    for (size_t index = 0;; index++) {
      bool isThatFunction;
      const char *methodName = object_methods[index].name;
      if (methodName == NULL) {   // reached end of list
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

  PyObject *attrName = idToKey(cx, id);
  PyObject *item = PyDict_GetItemWithError(pyObject, attrName);
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

bool PyDictProxyHandler::set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::HandleValue v, JS::HandleValue receiver,
  JS::ObjectOpResult &result) const {
  JS::RootedValue *rootedV = new JS::RootedValue(cx, v);
  PyObject *attrName = idToKey(cx, id);
  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  if (PyDict_SetItem(pyObject, attrName, pyTypeFactory(cx, global, rootedV)->getPyObject())) {
    return result.failCantSetInterposed(); // raises JS exception
  }
  return result.succeed();
}

bool PyDictProxyHandler::enumerate(JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

bool PyDictProxyHandler::hasOwn(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
  PyObject *attrName = idToKey(cx, id);
  *bp = PyDict_Contains(pyObject, attrName) == 1;
  return true;
}

bool PyDictProxyHandler::getOwnEnumerablePropertyKeys(
  JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

// TODO not needed at this time since only called as part of cleanup function's js::DestroyContext call which is only called at cpython exit Py_AtExit in PyInit_pythonmonkey
// put in some combination of the commented-out code below
void PyDictProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {
  /*PyThreadState *state = PyThreadState_Get(); 
  PyThreadState *state = PyGILState_GetThisThreadState();
  if (state) {
    PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
    PyGILState_STATE state = PyGILState_Ensure();
    Py_DECREF(self);
    PyGILState_Release(state);
  }*/
}

bool PyDictProxyHandler::defineProperty(JSContext *cx, JS::HandleObject proxy,
  JS::HandleId id,
  JS::Handle<JS::PropertyDescriptor> desc,
  JS::ObjectOpResult &result) const {
  // Block direct `Object.defineProperty` since we already have the `set` method
  return result.failInvalidDescriptor();
}

bool PyDictProxyHandler::getBuiltinClass(JSContext *cx, JS::HandleObject proxy,
  js::ESClass *cls) const {
  *cls = js::ESClass::Object;
  return true;
}