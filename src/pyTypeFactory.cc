#include "include/pyTypeFactory.hh"

#include "include/BoolType.hh"
#include "include/DateType.hh"
#include "include/DictType.hh"
#include "include/FloatType.hh"
#include "include/FuncType.hh"
#include "include/IntType.hh"
#include "include/jsTypeFactory.hh"
#include "include/ListType.hh"
#include "include/NoneType.hh"
#include "include/NullType.hh"
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

PyType *pyTypeFactory(JSContext *cx, JS::Rooted<JSObject *> *global, JS::Rooted<JS::Value> *rval) {
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
      // @TODO (Caleb Aikens) need to determine if this is one of OUR ProxyObjects somehow
      // consider putting a special value in one of the private slots when creating a PyProxyHandler
    }
    js::ESClass cls;
    JS::GetBuiltinClass(cx, obj, &cls);
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
    case js::ESClass::Function: {
        PyObject *JSCxGlobalFuncTuple = Py_BuildValue("(lll)", (uint64_t)cx, (uint64_t)global, (uint64_t)rval);
        PyObject *pyFunc = PyCFunction_New(&callJSFuncDef, JSCxGlobalFuncTuple);
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
    }
    return new DictType(cx, *rval);
  }
  else if (rval->isMagic()) {
    printf("magic type is not handled by PythonMonkey yet");
  }

  std::string errorString("pythonmonkey cannot yet convert Javascript value of: ");
  JS::RootedString str(cx, rval->toString());
  errorString += JS_EncodeStringToUTF8(cx, str).get();
  PyErr_SetString(PyExc_TypeError, errorString.c_str());
}

static PyObject *callJSFunc(PyObject *JSCxGlobalFuncTuple, PyObject *args) {
  // TODO (Caleb Aikens) convert PyObject *args to JS::Rooted<JS::ValueArray> JSargs
  JSContext *JScontext = (JSContext *)PyLong_AsLongLong(PyTuple_GetItem(JSCxGlobalFuncTuple, 0));
  JS::RootedObject *globalObject = (JS::RootedObject *)PyLong_AsLongLong(PyTuple_GetItem(JSCxGlobalFuncTuple, 1));
  JS::RootedValue *JSFuncValue = (JS::RootedValue *)PyLong_AsLongLong(PyTuple_GetItem(JSCxGlobalFuncTuple, 2));

  JS::RootedVector<JS::Value> JSargsVector(JScontext);
  for (size_t i = 0; i < PyTuple_Size(args); i++) {
    JS::Value jsValue = jsTypeFactory(JScontext, PyTuple_GetItem(args, i));
    if (PyErr_Occurred()) { // Check if an exception has already been set in the flow of control
      return NULL; // Fail-fast
    }
    JSargsVector.append(jsValue);
  }

  JS::HandleValueArray JSargs(JSargsVector);
  JS::Rooted<JS::Value> *JSreturnVal = new JS::Rooted<JS::Value>(JScontext);
  if (!JS_CallFunctionValue(JScontext, *globalObject, *JSFuncValue, JSargs, JSreturnVal)) {
    setSpiderMonkeyException(JScontext);
    return NULL;
  }

  return pyTypeFactory(JScontext, globalObject, JSreturnVal)->getPyObject();
}