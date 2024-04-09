/**
 * @file PyIterableProxyHandler.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS proxy iterators.
 * @date 2024-04-08
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */


#include "include/PyIterableProxyHandler.hh"

#include "include/jsTypeFactory.hh"

#include <jsapi.h>

#include <Python.h>



const char PyIterableProxyHandler::family = 0;

bool PyIterableProxyHandler::iterable_next(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject thisObj(cx);
  if (!args.computeThis(cx, &thisObj)) return false;

  PyObject *it = JS::GetMaybePtrFromReservedSlot<PyObject>(thisObj, PyObjectSlot);

  JS::RootedObject result(cx, JS_NewPlainObject(cx));

  PyObject *(*iternext)(PyObject *) = *Py_TYPE(it)->tp_iternext;

  PyObject *item = iternext(it);

  if (item == NULL) {
    if (PyErr_Occurred()) {
      if (PyErr_ExceptionMatches(PyExc_StopIteration) ||
          PyErr_ExceptionMatches(PyExc_SystemError)) {      // TODO this handles a result like   SystemError: Objects/dictobject.c:1778: bad argument to internal function. Why are we getting that?
        PyErr_Clear();
      }
      else {
        return NULL;
      }
    }

    JS::RootedValue done(cx, JS::BooleanValue(true));
    if (!JS_SetProperty(cx, result, "done", done)) return false;
    args.rval().setObject(*result);
    return result;
  }

  JS::RootedValue done(cx, JS::BooleanValue(false));
  if (!JS_SetProperty(cx, result, "done", done)) return false;

  JS::RootedValue value(cx, jsTypeFactory(cx, item));
  if (!JS_SetProperty(cx, result, "value", value)) return false;

  args.rval().setObject(*result);
  return true;
}

JSMethodDef PyIterableProxyHandler::iterable_methods[] = {
  {"next", PyIterableProxyHandler::iterable_next, 0},
  {NULL, NULL, 0}
};

bool PyIterableProxyHandler::getOwnPropertyDescriptor(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
) const {
  if (id.isString()) {
    for (size_t index = 0;; index++) {
      bool isThatFunction;
      const char *methodName = iterable_methods[index].name;
      if (methodName == NULL) {
        break;
      }
      else if (JS_StringEqualsAscii(cx, id.toString(), methodName, &isThatFunction) && isThatFunction) {
        JSFunction *newFunction = JS_NewFunction(cx, iterable_methods[index].call, iterable_methods[index].nargs, 0, NULL);
        if (!newFunction) return false;
        JS::RootedObject funObj(cx, JS_GetFunctionObject(newFunction));
        desc.set(mozilla::Some(
          JS::PropertyDescriptor::Data(
            JS::ObjectValue(*funObj),
            {JS::PropertyAttribute::Enumerable}
          )
        ));
        return true;
      }
    }
  }

  PyObject *attrName = idToKey(cx, id);
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  PyObject *item = PyDict_GetItemWithError(self, attrName);

  return handleGetOwnPropertyDescriptor(cx, id, desc, item);
}