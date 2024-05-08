/**
 * @file FuncType.cc
 * @author Caleb Aikens (caleb@distributive.network), Giovanni Tedesco (giovanni@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct representing python functions
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#include "include/FuncType.hh"
#include "include/JSFunctionProxy.hh"

#include <jsapi.h>


PyObject *FuncType::getPyObject(JSContext *cx, JS::HandleValue fval) {
  JSFunctionProxy *proxy = (JSFunctionProxy *)PyObject_CallObject((PyObject *)&JSFunctionProxyType, NULL);
  proxy->jsFunc->set(&fval.toObject());
  return (PyObject *)proxy;
}