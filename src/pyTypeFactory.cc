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
#include "include/PyType.hh"
#include "include/setSpiderMonkeyException.hh"
#include "include/StrType.hh"
#include "include/TupleType.hh"
#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include <jsapi.h>
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
  PyType *returnValue = NULL;
  if (rval->isUndefined()) {
    returnValue = new NoneType();
  }
  else if (rval->isNull()) {
    returnValue = new NullType();
  }
  else if (rval->isBoolean()) {
    returnValue = new BoolType(rval->toBoolean());
  }
  else if (rval->isNumber()) {
    returnValue = new FloatType(rval->toNumber());
  }
  else if (rval->isString()) {
    returnValue = new StrType(cx, rval->toString());
    memoizePyTypeAndGCThing(returnValue, *rval); // TODO (Caleb Aikens) consider putting this in the StrType constructor
  }
  else if (rval->isSymbol()) {
    printf("symbol type is not handled by PythonMonkey yet");
  }
  else if (rval->isBigInt()) {
    returnValue = new IntType(cx, rval->toBigInt());
  }
  else if (rval->isObject()) {
    JS::Rooted<JSObject *> obj(cx);
    JS_ValueToObject(cx, *rval, &obj);
    js::ESClass cls;
    JS::GetBuiltinClass(cx, obj, &cls);
    switch (cls) {
    case js::ESClass::Boolean: {
        // TODO (Caleb Aikens): refactor out all `js::Unbox` calls
        // TODO (Caleb Aikens): refactor using recursive call to `pyTypeFactory`
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        returnValue = new BoolType(unboxed.toBoolean());
        break;
      }
    case js::ESClass::Date: {
        returnValue = new DateType(cx, obj);
        break;
      }
    case js::ESClass::Promise: {
        returnValue = new PromiseType(cx, obj);
        break;
      }
    case js::ESClass::Error: {
        returnValue = new ExceptionType(cx, obj);
        break;
      }
    case js::ESClass::Function: {
        PyObject *jsCxThisFuncTuple = Py_BuildValue("(lll)", (uint64_t)cx, (uint64_t)thisObj, (uint64_t)rval);
        PyObject *pyFunc = PyCFunction_New(&callJSFuncDef, jsCxThisFuncTuple);
        returnValue = new FuncType(pyFunc);
        memoizePyTypeAndGCThing(returnValue, *rval); // TODO (Caleb Aikens) consider putting this in the FuncType constructor
        break;
      }
    case js::ESClass::Number: {
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        returnValue = new FloatType(unboxed.toNumber());
        break;
      }
    case js::ESClass::BigInt: {
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        returnValue = new IntType(cx, unboxed.toBigInt());
        break;
      }
    case js::ESClass::String: {
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        returnValue = new StrType(cx, unboxed.toString());
        memoizePyTypeAndGCThing(returnValue, *rval);   // TODO (Caleb Aikens) consider putting this in the StrType constructor
        break;
      }
    default: {
        if (BufferType::isSupportedJsTypes(obj)) { // TypedArray or ArrayBuffer
          // TODO (Tom Tang): ArrayBuffers have cls == js::ESClass::String
          returnValue = new BufferType(cx, obj);
          // if (returnValue->getPyObject() != nullptr) memoizePyTypeAndGCThing(returnValue, *rval);
        } else {
          printf("objects of this type (%d) are not handled by PythonMonkey yet\n", cls);
        }
      }
    }
  }
  else if (rval->isMagic()) {
    printf("magic type is not handled by PythonMonkey yet\n");
  }

  return returnValue;
}

static PyObject *callJSFunc(PyObject *jsCxThisFuncTuple, PyObject *args) {
  // TODO (Caleb Aikens) convert PyObject *args to JS::Rooted<JS::ValueArray> JSargs
  JSContext *cx = (JSContext *)PyLong_AsLongLong(PyTuple_GetItem(jsCxThisFuncTuple, 0));
  JS::RootedObject *thisObj = (JS::RootedObject *)PyLong_AsLongLong(PyTuple_GetItem(jsCxThisFuncTuple, 1));
  JS::RootedValue *jsFunc = (JS::RootedValue *)PyLong_AsLongLong(PyTuple_GetItem(jsCxThisFuncTuple, 2));

  JS::RootedVector<JS::Value> jsArgsVector(cx);
  for (size_t i = 0; i < PyTuple_Size(args); i++) {
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