#include "include/DictType.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"

#include <jsfriendapi.h>
#include <jsapi.h>
#include <js/Equality.h>

#include <Python.h>

#include <string>
#include <iostream>

typedef std::unordered_map<const JS::Value *, PyObject *>::iterator subObjectIterator;

DictType::DictType() {
  this->pyObject = PyDict_New();
}

DictType::DictType(PyObject *object) : PyType(object) {}

DictType::DictType(JSContext *cx, JS::Handle<JSObject *> global, JS::Handle<JS::Value> jsObject) {
  std::unordered_map<const JS::Value *, PyObject *> subObjectMap;
  init(cx, global, jsObject, subObjectMap);
}

DictType::DictType(JSContext *cx, JS::Handle<JSObject *> global, JS::Handle<JS::Value> jsObject, std::unordered_map<const JS::Value *, PyObject *> &subObjectsMap) {
  init(cx, global, jsObject, subObjectsMap);
}

void DictType::init(JSContext *cx, JS::Handle<JSObject *> global, JS::Handle<JS::Value> jsObject, std::unordered_map<const JS::Value *, PyObject *> &subObjectsMap) {
  for (auto it: subObjectsMap) {
    bool *isEqual;
    JS::RootedValue rval(cx, *it.first);
    if (JS::StrictlyEqual(cx, rval, jsObject, isEqual) && *isEqual) { // if object has already been coerced, need to avoid reference cycle
      this->pyObject = it.second;
      Py_INCREF(this->pyObject);
      return;
    }
  }

  this->pyObject = PyDict_New();
  subObjectsMap.insert({{jsObject.address(), this->pyObject}});

  JS::RootedObject globalObject(cx, global);
  JS::Rooted<JSObject *> jsObjectObj(cx);
  JS_ValueToObject(cx, jsObject, &jsObjectObj);
  /* @TODO (Caleb Aikens)
     Need to consider consequences of key types
     Javascript keys can be Strings or Symbols (do we need to handle Symbol coercion?)
     Python keys can be any immutable type
   */
  JS::RootedIdVector props(cx);
  if (!js::GetPropertyKeys(cx, jsObjectObj, JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS, &props)) {
    Py_DECREF(this->pyObject);
    this->pyObject = NULL;
    return;
  }

  for (size_t i = 0; i < props.length(); i++) {
    JS::HandleId id = props[i];
    JS::RootedValue *value = new JS::RootedValue(cx);
    if (id.isString()) { // @TODO (Caleb Aikens) handle non-String keys (Symbols, Ints(?) and Magic)
      if (!JS_GetPropertyById(cx, jsObjectObj, id, value)) {
        Py_DECREF(this->pyObject);
        this->pyObject = NULL;
        return;
      }
      JS::RootedValue keyValue(cx);
      keyValue.setString(id.toString());
      PyType *pyKey = checkJSMemo(keyValue);
      PyType *pyVal = checkJSMemo(*value);
      if (!pyKey) {
        pyKey = pyTypeFactory(cx, &globalObject, &keyValue);
      }
      if (!pyVal && value->isObject()) {
        JS::Rooted<JSObject *> valueObj(cx);
        JS_ValueToObject(cx, *value, &valueObj);
        js::ESClass cls;
        JS::GetBuiltinClass(cx, valueObj, &cls);
        if (cls == js::ESClass::Object) { // generic non-boxing object, need to worry about reference cycles
          pyVal = new DictType(cx, global, *value, subObjectsMap);
        }
      }
      if (!pyVal) {
        pyVal = pyTypeFactory(cx, &globalObject, value);
      }
      PyDict_SetItem(this->pyObject, pyKey->getPyObject(), pyVal->getPyObject());
    }
  }
}

void DictType::set(PyType *key, PyType *value) {
  PyDict_SetItem(this->pyObject, key->getPyObject(), value->getPyObject());
}

PyType *DictType::get(PyType *key) const {
  PyObject *retrieved_object = PyDict_GetItem(this->pyObject, key->getPyObject());
  return retrieved_object != NULL ? pyTypeFactory(retrieved_object) : nullptr;
}