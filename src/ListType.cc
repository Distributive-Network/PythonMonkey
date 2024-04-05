/**
 * @file ListType.cc
 * @author Caleb Aikens (caleb@distributive.network), Giovanni Tedesco (giovanni@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing python lists
 * @date 2022-08-18
 *
 * @copyright Copyright (c) 2022, 2023, 2024 Distributive Corp
 *
 */


#include "include/ListType.hh"
#include "include/PyType.hh"
#include "include/JSArrayProxy.hh"

#include <Python.h>


ListType::ListType() : PyType(PyList_New(0)) {}

ListType::ListType(PyObject *object) : PyType(object) {}

ListType::ListType(JSContext *cx, JS::HandleObject jsArrayObj) {
  JSArrayProxy *proxy = (JSArrayProxy *)PyObject_CallObject((PyObject *)&JSArrayProxyType, NULL);
  if (proxy != NULL) {
    proxy->jsArray = new JS::PersistentRootedObject(cx);
    proxy->jsArray->set(jsArrayObj);
    this->pyObject = (PyObject *)proxy;
  }
}