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

#include "include/JSArrayProxy.hh"


PyObject *ListType::getPyObject(JSContext *cx, JS::HandleObject jsArrayObj) {
  JSArrayProxy *proxy = (JSArrayProxy *)PyObject_CallObject((PyObject *)&JSArrayProxyType, NULL);
  if (proxy != NULL) {
    proxy->jsArray = new JS::PersistentRootedObject(cx);
    proxy->jsArray->set(jsArrayObj);
    return (PyObject *)proxy;
  }
  return NULL;
}