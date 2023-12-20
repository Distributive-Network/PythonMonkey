#include "include/DictType.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/JSObjectProxy.hh"
#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"

#include <jsfriendapi.h>
#include <jsapi.h>

#include <Python.h>


DictType::DictType() {
  this->pyObject = PyDict_New();
}

DictType::DictType(PyObject *object) : PyType(object) {}

DictType::DictType(JSContext *cx, JS::Handle<JS::Value> jsObject) {
  JSObjectProxy *proxy = (JSObjectProxy *)PyObject_CallObject((PyObject *)&JSObjectProxyType, NULL);
  JS::RootedObject obj(cx);
  JS_ValueToObject(cx, jsObject, &obj);
  proxy->jsObject.set(obj);
  this->pyObject = (PyObject *)proxy;
}