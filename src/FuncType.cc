/**
 * @file FuncType.cc
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Struct representing python functions
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2022 Distributive Corp.
 *
 */

#include "include/FuncType.hh"
#include "include/JSFunctionProxy.hh"
#include "include/PyType.hh"

#include <jsapi.h>

#include <Python.h>

FuncType::FuncType(PyObject *object) : PyType(object) {}

FuncType::FuncType(JSContext *cx, JS::HandleValue fval) {
  JSFunctionProxy *proxy = (JSFunctionProxy *)PyObject_CallObject((PyObject *)&JSFunctionProxyType, NULL);
  proxy->jsFunc->set(&fval.toObject());
  this->pyObject = (PyObject *)proxy;
}

const char *FuncType::getValue() const {
  return PyUnicode_AsUTF8(PyObject_GetAttrString(pyObject, "__name__"));
}