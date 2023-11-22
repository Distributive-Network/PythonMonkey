/**
 * @file PyProxyHandler.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for creating JS proxy objects. Used by DictType for object coercion
 * @version 0.1
 * @date 2023-04-20
 *
 * Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/PyProxyHandler.hh"

#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/StrType.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Conversions.h>
#include <js/Proxy.h>

#include <Python.h>

PyObject *idToKey(JSContext *cx, JS::HandleId id) {
  JS::RootedValue idv(cx, js::IdToValue(id));
  JS::RootedString idStr(cx);
  if (!id.isSymbol()) { // `JS::ToString` returns `nullptr` for JS symbols
    idStr = JS::ToString(cx, idv);
  } else {
    // TODO (Tom Tang): Revisit this once we have Symbol coercion support
    // FIXME (Tom Tang): key collision for symbols without a description string, or pure strings look like "Symbol(xxx)"
    idStr = JS_ValueToSource(cx, idv);
  }

  // We convert all types of property keys to string
  auto chars = JS_EncodeStringToUTF8(cx, idStr);
  return PyUnicode_FromString(chars.get());
}

bool idToIndex(JSContext *cx, JS::HandleId id, Py_ssize_t *index) {
  if (id.isInt()) { // int-like strings have already been automatically converted to ints
    *index = id.toInt();
    return true;
  } else {
    return false; // fail
  }
}

const char PyProxyHandler::family = 0;

bool PyProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const {
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

bool PyProxyHandler::delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::ObjectOpResult &result) const {
  PyObject *attrName = idToKey(cx, id);
  if (PyDict_DelItem(pyObject, attrName) < 0) {
    return result.failCantDelete(); // raises JS exception
  }
  return result.succeed();
}

bool PyProxyHandler::has(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
  return hasOwn(cx, proxy, id, bp);
}

bool PyProxyHandler::get(JSContext *cx, JS::HandleObject proxy,
  JS::HandleValue receiver, JS::HandleId id,
  JS::MutableHandleValue vp) const {
  PyObject *attrName = idToKey(cx, id);
  PyObject *p = PyDict_GetItemWithError(pyObject, attrName);
  if (!p) { // NULL if the key is not present
    vp.setUndefined(); // JS objects return undefined for nonpresent keys
  } else {
    vp.set(jsTypeFactory(cx, p));
  }
  return true;
}

bool PyProxyHandler::getOwnPropertyDescriptor(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
) const {
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

bool PyProxyHandler::set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
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

bool PyProxyHandler::enumerate(JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

bool PyProxyHandler::hasOwn(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
  PyObject *attrName = idToKey(cx, id);
  *bp = PyDict_Contains(pyObject, attrName) == 1;
  return true;
}

bool PyProxyHandler::getOwnEnumerablePropertyKeys(
  JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

// @TODO (Caleb Aikens) implement this
void PyProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {}

bool PyProxyHandler::defineProperty(JSContext *cx, JS::HandleObject proxy,
  JS::HandleId id,
  JS::Handle<JS::PropertyDescriptor> desc,
  JS::ObjectOpResult &result) const {
  // Block direct `Object.defineProperty` since we already have the `set` method
  return result.failInvalidDescriptor();
}

bool PyBaseProxyHandler::getPrototypeIfOrdinary(JSContext *cx, JS::HandleObject proxy,
  bool *isOrdinary,
  JS::MutableHandleObject protop) const {
  // We don't have a custom [[GetPrototypeOf]]
  *isOrdinary = true;
  protop.set(js::GetStaticPrototype(proxy));
  return true;
}

bool PyBaseProxyHandler::preventExtensions(JSContext *cx, JS::HandleObject proxy,
  JS::ObjectOpResult &result) const {
  result.succeed();
  return true;
}

bool PyBaseProxyHandler::isExtensible(JSContext *cx, JS::HandleObject proxy,
  bool *extensible) const {
  *extensible = false;
  return true;
}
