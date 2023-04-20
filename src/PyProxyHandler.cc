#include "include/PyProxyHandler.hh"

#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/StrType.hh"

#include <jsapi.h>
#include <js/Proxy.h>

#include <Python.h>

PyProxyHandler::PyProxyHandler(PyObject *pyObj) : js::BaseProxyHandler(NULL), pyObject(pyObj) {}

bool PyProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
    PyObject *keys = PyDict_Keys(pyObject);
    for (size_t i = 0; i < PyList_Size(keys); i++) {
      props.append(jsTypeFactory(cx, PyList_GetItem(pyObject, i)));
    }
    return true;
  }

bool PyProxyHandler::delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::ObjectOpResult &result) const {
    PyObject *attrName = (new StrType(cx, id.toString()))->getPyObject();
    if (PyObject_DelAttr(pyObject, attrName) < 0) {
      // @TODO (Caleb Aikens) raise exception
      return false;
    }
    return true;
  }

bool PyProxyHandler::has(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
    return hasOwn(cx, proxy, id, bp);
  }

bool PyProxyHandler::get(JSContext *cx, JS::HandleObject proxy,
  JS::HandleValue receiver, JS::HandleId id,
  JS::MutableHandleValue vp) const {
    PyObject *attrName = (new StrType(cx, id.toString()))->getPyObject();
    PyObject *p = PyObject_GetAttr(pyObject, attrName);
    if (!p) {
      // @TODO (Caleb Aikens) raise exception
      return false;
    }
    vp.set(jsTypeFactory(cx, p));
    return true;
  }

bool PyProxyHandler::set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::HandleValue v, JS::HandleValue receiver,
  JS::ObjectOpResult &result) const {
    JS::RootedValue *rootedV = new JS::RootedValue(v);
    PyObject *attrName = (new StrType(cx, id.toString()))->getPyObject();
    JS::RootedObject *global = new JS::RootedObject(JS::GetNonCCWObjectGlobal(proxy));
    if (PyObject_SetAttr(pyObject, attrName, pyTypeFactory(cx, global, rootedV)->getPyObject())) {
      // @TODO (Caleb Aikens) raise exception
      return false;
    }
    // @TODO (Caleb Aikens) read about what you're supposed to do with receiver and result
    return true;
  }

// @TODO (Caleb Aikens) implement this
bool PyProxyHandler::enumerate(JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {}

bool PyProxyHandler::hasOwn(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
    PyObject *attrName = (new StrType(cx, id.toString()))->getPyObject();
    *bp = PyObject_HasAttr(pyObject, attrName);
    return true;
  }

// @TODO (Caleb Aikens) implement this
bool PyProxyHandler::getOwnEnumerablePropertyKeys(
  JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {}

// @TODO (Caleb Aikens) implement this
void PyProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {}