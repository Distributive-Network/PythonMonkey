#include "include/FuncType.hh"

#include "include/PyType.hh"
#include "include/JSFunctionProxy.hh"

#include <jsapi.h>

#include <Python.h>

FuncType::FuncType(PyObject *object) : PyType(object) {}

FuncType::FuncType(JSContext *cx, JS::Handle<JS::Value> jsFunction) {
  JSFunctionProxy *proxy = (JSFunctionProxy *)PyObject_CallObject((PyObject *)&JSFunctionProxyType, NULL);
  proxy->jsFunction = new JS::PersistentRootedObject(cx);
  JS_ValueToObject(cx, jsFunction, proxy->jsFunction);
  this->pyObject = (PyObject *)proxy;
  Py_INCREF(this->pyObject);
}
const char *FuncType::getValue() const {
  return PyUnicode_AsUTF8(PyObject_GetAttrString(pyObject, "__name__"));
}