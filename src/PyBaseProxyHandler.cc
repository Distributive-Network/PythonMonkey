/**
 * @file PyBaseProxyHandler.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS proxy objects
 * @date 2023-04-20
 *
 * @copyright Copyright (c) 2023-2024 Distributive Corp.
 *
 */


#include "include/PyBaseProxyHandler.hh"

#include <jsapi.h>

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