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

const char PyDictProxyHandler::family = 0;

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

bool PyDictProxyHandler::get(JSContext *cx, JS::HandleObject proxy,
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

bool PyDictProxyHandler::getOwnPropertyDescriptor(
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

void PyDictProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {
  Py_DECREF(pyObject);
}

bool PyDictProxyHandler::defineProperty(JSContext *cx, JS::HandleObject proxy,
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

const char PyListProxyHandler::family = 0;

bool PyListProxyHandler::getOwnPropertyDescriptor(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
) const {
  // We're trying to get the "length" property
  bool isLengthProperty;
  if (id.isString() && JS_StringEqualsLiteral(cx, id.toString(), "length", &isLengthProperty) && isLengthProperty) {
    // proxy.length = len(pyObject)
    desc.set(mozilla::Some(
      JS::PropertyDescriptor::Data(
        JS::Int32Value(PySequence_Size(pyObject))
      )
    ));
    return true;
  }

  // We're trying to get an item
  Py_ssize_t index;
  PyObject *item;
  if (idToIndex(cx, id, &index) && (item = PySequence_GetItem(pyObject, index))) {
    desc.set(mozilla::Some(
      JS::PropertyDescriptor::Data(
        jsTypeFactory(cx, item),
        {JS::PropertyAttribute::Writable, JS::PropertyAttribute::Enumerable}
      )
    ));
  } else { // item not found in list, or not an int-like property key
    desc.set(mozilla::Nothing());
  }
  return true;
}

void PyListProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {
  Py_DECREF(pyObject);
}

bool PyListProxyHandler::defineProperty(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::Handle<JS::PropertyDescriptor> desc, JS::ObjectOpResult &result
) const {
  Py_ssize_t index;
  if (!idToIndex(cx, id, &index)) { // not an int-like property key
    return result.failBadIndex();
  }

  if (desc.isAccessorDescriptor()) { // containing getter/setter
    return result.failNotDataDescriptor();
  }
  if (!desc.hasValue()) {
    return result.failInvalidDescriptor();
  }

  // FIXME (Tom Tang): memory leak
  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  JS::RootedValue *itemV = new JS::RootedValue(cx, desc.value());
  PyObject *item = pyTypeFactory(cx, global, itemV)->getPyObject();
  if (PySequence_SetItem(pyObject, index, item) < 0) {
    return result.failBadIndex();
  }
  return result.succeed();
}

bool PyListProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const {
  // Modified from https://hg.mozilla.org/releases/mozilla-esr102/file/3b574e1/dom/base/RemoteOuterWindowProxy.cpp#l137
  int32_t length = PySequence_Size(pyObject);
  if (!props.reserve(length + 1)) {
    return false;
  }
  // item indexes
  for (int32_t i = 0; i < length; ++i) {
    props.infallibleAppend(JS::PropertyKey::Int(i));
  }
  // the "length" property
  props.infallibleAppend(JS::PropertyKey::NonIntAtom(JS_AtomizeString(cx, "length")));
  return true;
}

bool PyListProxyHandler::delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id, JS::ObjectOpResult &result) const {
  Py_ssize_t index;
  if (!idToIndex(cx, id, &index)) {
    return result.failBadIndex(); // report failure
  }

  // Set to undefined instead of actually deleting it
  if (PySequence_SetItem(pyObject, index, Py_None) < 0) {
    return result.failCantDelete(); // report failure
  }
  return result.succeed(); // report success
}

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

bool PyObjectProxyHandler::get(JSContext *cx, JS::HandleObject proxy,
  JS::HandleValue receiver, JS::HandleId id,
  JS::MutableHandleValue vp) const {
  PyObject *attrName = idToKey(cx, id);
  if (!PyObject_HasAttr(pyObject, attrName)) {
    vp.setUndefined(); // JS objects return undefined for nonpresent keys
    return true;
  }

  PyObject *p = PyObject_GetAttr(pyObject, attrName);
  vp.set(jsTypeFactory(cx, p));
  return true;
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
  JS::RootedValue *rootedV = new JS::RootedValue(cx, v);
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
  Py_DECREF(pyObject);
}

bool PyObjectProxyHandler::defineProperty(JSContext *cx, JS::HandleObject proxy,
  JS::HandleId id,
  JS::Handle<JS::PropertyDescriptor> desc,
  JS::ObjectOpResult &result) const {
  // Block direct `Object.defineProperty` since we already have the `set` method
  return result.failInvalidDescriptor();
}