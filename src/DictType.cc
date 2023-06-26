#include "include/DictType.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/JSObjectProxy.hh"
#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"

#include <jsfriendapi.h>
#include <jsapi.h>
#include <js/Equality.h>

#include <Python.h>

#include <string>

typedef std::unordered_map<const JS::Value *, PyObject *>::iterator subObjectIterator;

DictType::DictType() {
  this->pyObject = PyDict_New();
}

DictType::DictType(PyObject *object) : PyType(object) {}

DictType::DictType(JSContext *cx, JS::Handle<JS::Value> jsObject) {
  JSObjectProxy *proxy = PyObject_New(JSObjectProxy, &JSObjectProxyType);
  JS::RootedObject obj(cx);
  JS_ValueToObject(cx, jsObject, &obj);
  proxy->jsObject.set(obj);
  this->pyObject = (PyObject *)proxy;
}

void DictType::set(PyType *key, PyType *value) {
  PyDict_SetItem(this->pyObject, key->getPyObject(), value->getPyObject());
}

PyType *DictType::get(PyType *key) const {
  PyObject *retrieved_object = PyDict_GetItem(this->pyObject, key->getPyObject());
  return retrieved_object != NULL ? pyTypeFactory(retrieved_object) : nullptr;
}