/**
 * @file DictType.cc
 * @author Caleb Aikens (caleb@distributive.network), Giovanni Tedesco (giovanni@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct representing python dictionaries
 * @date 2022-08-10
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */


#include "include/DictType.hh"

#include "include/JSObjectProxy.hh"

#include <jsapi.h>


PyObject *DictType::getPyObject(JSContext *cx, JS::Handle<JS::Value> jsObject) {
  JSObjectProxy *proxy = (JSObjectProxy *)PyObject_CallObject((PyObject *)&JSObjectProxyType, NULL);
  if (proxy != NULL) {
    JS::RootedObject obj(cx);
    JS_ValueToObject(cx, jsObject, &obj);
    proxy->jsObject = new JS::PersistentRootedObject(cx);
    proxy->jsObject->set(obj);
    return (PyObject *)proxy;
  }
  return NULL;
}