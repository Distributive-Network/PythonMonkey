/**
 * @file DictType.cc
 * @author Caleb Aikens (caleb@distributive.network), Giovanni Tedesco (giovanni@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct representing python dictionaries
 * @date 2022-08-10
 *
 * @copyright Copyright (c) 2022 Distributive Corp.
 *
 */


#include "include/DictType.hh"

#include "include/JSObjectProxy.hh"
#include "include/PyType.hh"

#include <jsfriendapi.h>
#include <jsapi.h>

#include <Python.h>


DictType::DictType() {
  this->pyObject = PyDict_New();
}

DictType::DictType(PyObject *object) : PyType(object) {}

DictType::DictType(JSContext *cx, JS::Handle<JS::Value> jsObject) {
  JSObjectProxy *proxy = (JSObjectProxy *)PyObject_CallObject((PyObject *)&JSObjectProxyType, NULL);
  if (proxy != NULL) {
    JS::RootedObject obj(cx);
    JS_ValueToObject(cx, jsObject, &obj);
    proxy->jsObject.set(obj);
    this->pyObject = (PyObject *)proxy;
  }
}