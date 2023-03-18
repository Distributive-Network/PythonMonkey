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
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        returnValue = new BoolType(unboxed.toBoolean());
        break;
      }
    case js::ESClass::Date: {
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        returnValue = new DateType(cx, obj);
        break;
      }
    case js::ESClass::Function: {
        PyObject *JSCxGlobalFuncTuple = Py_BuildValue("(lll)", (long)cx, (long)global, (long)rval);
        PyObject *pyFunc = PyCFunction_New(&callJSFuncDef, JSCxGlobalFuncTuple);
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
    case js::ESClass::String: {
        JS::RootedValue unboxed(cx);
        js::Unbox(cx, obj, &unboxed);
        returnValue = new StrType(cx, unboxed.toString());
        memoizePyTypeAndGCThing(returnValue, *rval);   // TODO (Caleb Aikens) consider putting this in the StrType constructor
        break;
      }
    default: {
        printf("objects of this type are not handled by PythonMonkey yet");
      }
    }
  }
  else if (rval->isMagic()) {
    printf("magic type is not handled by PythonMonkey yet");
  }

  return returnValue;
}

static PyObject *callJSFunc(PyObject *JSCxGlobalFuncTuple, PyObject *args) {
  // TODO (Caleb Aikens) convert PyObject *args to JS::Rooted<JS::ValueArray> JSargs
  JSContext *JScontext = (JSContext *)PyLong_AsLong(PyTuple_GetItem(JSCxGlobalFuncTuple, 0));
  JS::RootedObject *globalObject = (JS::RootedObject *)PyLong_AsLong(PyTuple_GetItem(JSCxGlobalFuncTuple, 1));
  JS::RootedValue *JSFuncValue = (JS::RootedValue *)PyLong_AsLong(PyTuple_GetItem(JSCxGlobalFuncTuple, 2));

  JS::RootedVector<JS::Value> JSargsVector(JScontext);
  for (size_t i = 0; i < PyTuple_Size(args); i++) {
    // TODO (Caleb Aikens) write an overload for jsTypeFactory to handle PyObjects directly
    JS::Value jsValue = jsTypeFactory(PyTuple_GetItem(args, i));
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