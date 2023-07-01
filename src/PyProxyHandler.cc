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
  JSString *idStr;
  if (!id.isSymbol()) { // `JS::ToString` returns `nullptr` for JS symbols
    idStr = JS::ToString(cx, idv);
  } else {
    // TODO (Tom Tang): Revisit this once we have Symbol coercion support
    // FIXME (Tom Tang): key collision for symbols without a description string, or pure strings look like "Symbol(xxx)"
    idStr = JS_ValueToSource(cx, idv);
  }
  // We convert all types of property keys to string
  return StrType(cx, idStr).getPyObject();
}

bool idToIndex(JSContext *cx, JS::HandleId id, Py_ssize_t *index) {
  if (id.isInt()) { // int-like strings have already been automatically converted to ints
    *index = id.toInt();
    return true;
  } else {
    return false; // fail
  }
}

bool PyProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  PyObject *keys = PyDict_Keys(pyObject);
  for (size_t i = 0; i < PyList_Size(keys); i++) {
    PyObject *key = PyList_GetItem(keys, i);
    JS::RootedValue jsKey(cx, jsTypeFactory(cx, key));
    JS::RootedId jsId(cx);
    if (!JS_ValueToId(cx, jsKey, &jsId)) {
      // @TODO (Caleb Aikens) raise exception
    }
    if (!props.append(jsId)) {
      // @TODO (Caleb Aikens) raise exception
    }
  }
  return true;
}

bool PyProxyHandler::delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::ObjectOpResult &result) const {
  PyObject *attrName = idToKey(cx, id);
  if (PyDict_DelItem(pyObject, attrName) < 0) {
    // @TODO (Caleb Aikens) raise exception
    return false;
  }
  return true;
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

bool PyProxyHandler::set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::HandleValue v, JS::HandleValue receiver,
  JS::ObjectOpResult &result) const {
  JS::RootedValue *rootedV = new JS::RootedValue(cx, v);
  PyObject *attrName = idToKey(cx, id);
  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  if (PyDict_SetItem(pyObject, attrName, pyTypeFactory(cx, global, rootedV)->getPyObject())) {
    // @TODO (Caleb Aikens) raise exception
    return false;
  }
  // @TODO (Caleb Aikens) read about what you're supposed to do with receiver and result
  return true;
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
  if (desc.hasConfigurable() && desc.configurable()) {}

  // JS::RootedValue *rootedV = new JS::RootedValue(cx, v);
  // PyObject *attrName = idToKey(cx, id);
  // JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  // if (PyDict_SetItem(pyObject, attrName, pyTypeFactory(cx, global, rootedV)->getPyObject())) {
  //   // @TODO (Caleb Aikens) raise exception
  //   return false;
  // }
  // // @TODO (Caleb Aikens) read about what you're supposed to do with receiver and result
  // return true;
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
  // TODO (Tom Tang): populate from `Py_ssize_t len = PySequence_Size(pyObject);` plus the "length" property
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
