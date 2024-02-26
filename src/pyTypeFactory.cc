/**
 * @file pyTypeFactory.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Function for wrapping arbitrary PyObjects into the appropriate PyType class, and coercing JS types to python types
 * @version 0.1
 * @date 2023-03-29
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "include/pyTypeFactory.hh"

#include "include/BoolType.hh"
#include "include/BufferType.hh"
#include "include/DateType.hh"
#include "include/DictType.hh"
#include "include/ExceptionType.hh"
#include "include/FloatType.hh"
#include "include/FuncType.hh"
#include "include/IntType.hh"
#include "include/jsTypeFactory.hh"
#include "include/ListType.hh"
#include "include/NoneType.hh"
#include "include/NullType.hh"
#include "include/PromiseType.hh"
#include "include/PyDictProxyHandler.hh"
#include "include/PyListProxyHandler.hh"
#include "include/PyType.hh"
#include "include/setSpiderMonkeyException.hh"
#include "include/StrType.hh"
#include "include/TupleType.hh"
#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Object.h>
#include <js/ValueArray.h>

#include <Python.h>

// TODO (Caleb Aikens) get below properties
static PyMethodDef callJSFuncDef = {"JSFunctionCallable", callJSFunc, METH_VARARGS, NULL};

PyType *pyTypeFactory(PyObject *object) {
  PyType *pyType;

  if (PyLong_Check(object)) {
    pyType = new IntType(object);
  }
  else if (PyUnicode_Check(object)) {
    pyType = new StrType(object);
  }
  else if (PyFunction_Check(object)) {
    pyType = new FuncType(object);
  }
  else if (PyDict_Check(object)) {
    pyType = new DictType(object);
  }
  else if (PyList_Check(object)) {
    pyType = new ListType(object);
  }
  else if (PyTuple_Check(object)) {
    pyType = new TupleType(object);
  }
  else {
    return nullptr;
  }

  return pyType;
}

PyType *pyTypeFactory(JSContext *cx, JS::Rooted<JSObject *> *thisObj, JS::Rooted<JS::Value> *rval) {
  if (rval->isUndefined()) {
    return new NoneType();
  }
  else if (rval->isNull()) {
    return new NullType();
  }
  else if (rval->isBoolean()) {
    return new BoolType(rval->toBoolean());
  }
  else if (rval->isNumber()) {
    return new FloatType(rval->toNumber());
  }
  else if (rval->isString()) {
    StrType *s = new StrType(cx, rval->toString());
    memoizePyTypeAndGCThing(s, *rval); // TODO (Caleb Aikens) consider putting this in the StrType constructor
    return s;
  }
  else if (rval->isSymbol()) {
    printf("symbol type is not handled by PythonMonkey yet");
  }
  else if (rval->isBigInt()) {
    return new IntType(cx, rval->toBigInt());
  }
  else if (rval->isObject()) {
    JS::Rooted<JSObject *> obj(cx);
    JS_ValueToObject(cx, *rval, &obj);
    if (JS::GetClass(obj)->isProxyObject()) {
      if (js::GetProxyHandler(obj)->family() == &PyDictProxyHandler::family) { // this is one of our proxies for python dicts
        return new DictType(((PyDictProxyHandler *)js::GetProxyHandler(obj))->pyObject);
      }
      if (js::GetProxyHandler(obj)->family() == &PyListProxyHandler::family) { // this is one of our proxies for python lists
        return new ListType(((PyListProxyHandler *)js::GetProxyHandler(obj))->pyObject);
      }
    }
    js::ESClass cls;
    JS::GetBuiltinClass(cx, obj, &cls);
    if (JS_ObjectIsBoundFunction(obj)) {
      cls = js::ESClass::Function; // In SpiderMonkey 115 ESR, bound function is no longer a JSFunction but a js::BoundFunctionObject.
                                   // js::ESClass::Function only assigns to JSFunction objects by JS::GetBuiltinClass.
    }
    switch (cls) {
    case js::ESClass::Boolean: {
        // TODO (Caleb Aikens): refactor out all `js::Unbox` calls
        // TODO (Caleb Aikens): refactor using recursive call to `pyTypeFactory`
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        return new BoolType(unboxed.toBoolean());
      }
    case js::ESClass::Date: {
        return new DateType(cx, obj);
      }
    case js::ESClass::Promise: {
        return new PromiseType(cx, obj);
      }
    case js::ESClass::Error: {
        return new ExceptionType(cx, obj);
      }
    case js::ESClass::Function: {
        PyObject *pyFunc;
        if (JS_IsNativeFunction(obj, callPyFunc)) { // It's a wrapped python function by us
          // Get the underlying python function from the 0th reserved slot
          JS::Value pyFuncVal = js::GetFunctionNativeReserved(obj, 0);
          pyFunc = (PyObject *)(pyFuncVal.toPrivate());
        } else {
          // FIXME (Tom Tang): `jsCxThisFuncTuple` and the tuple items are not going to be GCed
          PyObject *jsCxThisFuncTuple = PyTuple_Pack(3, PyLong_FromVoidPtr(cx), PyLong_FromVoidPtr(thisObj), PyLong_FromVoidPtr(rval));
          pyFunc = PyCFunction_New(&callJSFuncDef, jsCxThisFuncTuple);
        }
        FuncType *f = new FuncType(pyFunc);
        memoizePyTypeAndGCThing(f, *rval); // TODO (Caleb Aikens) consider putting this in the FuncType constructor
        return f;
      }
    case js::ESClass::Number: {
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        return new FloatType(unboxed.toNumber());
      }
    case js::ESClass::BigInt: {
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        return new IntType(cx, unboxed.toBigInt());
      }
    case js::ESClass::String: {
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        StrType *s = new StrType(cx, unboxed.toString());
        memoizePyTypeAndGCThing(s, *rval);   // TODO (Caleb Aikens) consider putting this in the StrType constructor
        return s;
      }
    case js::ESClass::Array: {
        return new ListType(cx, obj);
      }
    default: {
        if (BufferType::isSupportedJsTypes(obj)) { // TypedArray or ArrayBuffer
          // TODO (Tom Tang): ArrayBuffers have cls == js::ESClass::ArrayBuffer
          return new BufferType(cx, obj);
        }
      }
    }
    return new DictType(cx, *rval);
  }
  else if (rval->isMagic()) {
    printf("magic type is not handled by PythonMonkey yet\n");
  }

  std::string errorString("pythonmonkey cannot yet convert Javascript value of: ");
  JS::RootedString str(cx, JS::ToString(cx, *rval));
  errorString += JS_EncodeStringToUTF8(cx, str).get();
  PyErr_SetString(PyExc_TypeError, errorString.c_str());
  return NULL;
}

PyType *pyTypeFactorySafe(JSContext *cx, JS::Rooted<JSObject *> *thisObj, JS::Rooted<JS::Value> *rval) {
  PyType *v = pyTypeFactory(cx, thisObj, rval);
  if (PyErr_Occurred()) {
    // Clear Python error
    PyErr_Clear();
    // Return `pythonmonkey.null` on error
    return new NullType();
  }
  return v;
}

PyObject *callJSFunc(PyObject *jsCxThisFuncTuple, PyObject *args) {
  // TODO (Caleb Aikens) convert PyObject *args to JS::Rooted<JS::ValueArray> JSargs
  JSContext *cx = (JSContext *)PyLong_AsVoidPtr(PyTuple_GetItem(jsCxThisFuncTuple, 0));
  JS::RootedObject *thisObj = (JS::RootedObject *)PyLong_AsVoidPtr(PyTuple_GetItem(jsCxThisFuncTuple, 1));
  JS::RootedValue *jsFunc = (JS::RootedValue *)PyLong_AsVoidPtr(PyTuple_GetItem(jsCxThisFuncTuple, 2));

  JS::RootedVector<JS::Value> jsArgsVector(cx);
  Py_ssize_t tupleSize = PyTuple_Size(args);
  for (size_t i = 0; i < tupleSize; i++) {
    JS::Value jsValue = jsTypeFactory(cx, PyTuple_GetItem(args, i));
    if (PyErr_Occurred()) { // Check if an exception has already been set in the flow of control
      return NULL; // Fail-fast
    }
    jsArgsVector.append(jsValue);
  }

  JS::HandleValueArray jsArgs(jsArgsVector);
  JS::Rooted<JS::Value> *jsReturnVal = new JS::Rooted<JS::Value>(cx);
  if (!JS_CallFunctionValue(cx, *thisObj, *jsFunc, jsArgs, jsReturnVal)) {
    setSpiderMonkeyException(cx);
    return NULL;
  }

  return pyTypeFactory(cx, thisObj, jsReturnVal)->getPyObject();
}