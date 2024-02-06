#include "include/ListType.hh"
#include "include/PyType.hh"
#include "include/JSArrayProxy.hh"

#include <Python.h>


ListType::ListType() : PyType(PyList_New(0)) {}

ListType::ListType(PyObject *object) : PyType(object) {}

ListType::ListType(JSContext *cx, JS::HandleObject jsArrayObj) {
  JSArrayProxy *proxy = (JSArrayProxy *)PyObject_CallObject((PyObject *)&JSArrayProxyType, NULL);
  if (proxy != NULL) {
    proxy->jsArray.set(jsArrayObj);
    this->pyObject = (PyObject *)proxy;
  }
}