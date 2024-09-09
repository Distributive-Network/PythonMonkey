/**
 * @file pyTypeFactory.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Function for wrapping arbitrary PyObjects into the appropriate PyType class, and coercing JS types to python types
 * @date 2023-03-29
 *
 * @copyright 2023-2024 Distributive Corp.
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
#include "include/PyObjectProxyHandler.hh"
#include "include/PyIterableProxyHandler.hh"
#include "include/PyBytesProxyHandler.hh"
#include "include/setSpiderMonkeyException.hh"
#include "include/StrType.hh"
#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Object.h>
#include <js/ValueArray.h>

PyObject *pyTypeFactory(JSContext *cx, JS::HandleValue rval) {
  std::string errorString;

  if (rval.isUndefined()) {
    return NoneType::getPyObject();
  }
  else if (rval.isNull()) {
    return NullType::getPyObject();
  }
  else if (rval.isBoolean()) {
    return BoolType::getPyObject(rval.toBoolean());
  }
  else if (rval.isNumber()) {
    return FloatType::getPyObject(rval.toNumber());
  }
  else if (rval.isString()) {
    return StrType::getPyObject(cx, rval);
  }
  else if (rval.isSymbol()) {
    errorString = "symbol type is not handled by PythonMonkey yet.\n";
  }
  else if (rval.isBigInt()) {
    return IntType::getPyObject(cx, rval.toBigInt());
  }
  else if (rval.isObject()) {
    JS::Rooted<JSObject *> obj(cx);
    JS_ValueToObject(cx, rval, &obj);

    if (JS::GetClass(obj)->isProxyObject()) {
      if (js::GetProxyHandler(obj)->family() == &PyDictProxyHandler::family ||                // this is one of our proxies for python dicts
          js::GetProxyHandler(obj)->family() == &PyListProxyHandler::family ||                // this is one of our proxies for python lists
          js::GetProxyHandler(obj)->family() == &PyIterableProxyHandler::family ||            // this is one of our proxies for python iterables
          js::GetProxyHandler(obj)->family() == &PyObjectProxyHandler::family ||              // this is one of our proxies for python iterables
          js::GetProxyHandler(obj)->family() == &PyBytesProxyHandler::family) {               // this is one of our proxies for python bytes objects

        PyObject *pyObject = JS::GetMaybePtrFromReservedSlot<PyObject>(obj, PyObjectSlot);
        Py_INCREF(pyObject);
        return pyObject;
      }
    }

    js::ESClass cls;
    JS::GetBuiltinClass(cx, obj, &cls);
    if (JS_ObjectIsBoundFunction(obj)) {
      cls = js::ESClass::Function; // In SpiderMonkey 115 ESR, bound function is no longer a JSFunction but a js::BoundFunctionObject.
                                   // js::ESClass::Function only assigns to JSFunction objects by JS::GetBuiltinClass.
    }

    JS::RootedValue unboxed(cx);
    switch (cls) {
    case js::ESClass::Boolean:
    case js::ESClass::Number:
    case js::ESClass::BigInt:
    case js::ESClass::String:
      js::Unbox(cx, obj, &unboxed);
      return pyTypeFactory(cx, unboxed);
    case js::ESClass::Date:
      return DateType::getPyObject(cx, obj);
    case js::ESClass::Promise:
      return PromiseType::getPyObject(cx, obj);
    case js::ESClass::Error:
      return ExceptionType::getPyObject(cx, obj);
    case js::ESClass::Function: {
        if (JS_IsNativeFunction(obj, callPyFunc)) { // It's a wrapped python function by us
          // Get the underlying python function from the 0th reserved slot
          JS::Value pyFuncVal = js::GetFunctionNativeReserved(obj, 0);
          PyObject *pyFunc = (PyObject *)(pyFuncVal.toPrivate());
          Py_INCREF(pyFunc);
          return pyFunc;
        } else {
          return FuncType::getPyObject(cx, rval);
        }
      }
    case js::ESClass::Array:
      return ListType::getPyObject(cx, obj);
    default:
      if (BufferType::isSupportedJsTypes(obj)) { // TypedArray or ArrayBuffer
        // TODO (Tom Tang): ArrayBuffers have cls == js::ESClass::ArrayBuffer
        return BufferType::getPyObject(cx, obj);
      }
    }
    return DictType::getPyObject(cx, rval);
  }
  else if (rval.isMagic()) {
    errorString = "magic type is not handled by PythonMonkey yet.\n";
  }

  errorString += "pythonmonkey cannot yet convert Javascript value of: ";
  JSString *valToStr = JS::ToString(cx, rval);
  if (!valToStr) { // `JS::ToString` returns `nullptr` for JS symbols, see https://hg.mozilla.org/releases/mozilla-esr102/file/3b574e1/js/src/vm/StringType.cpp#l2208
    // TODO (Tom Tang): Revisit this once we have Symbol coercion support
    valToStr = JS_ValueToSource(cx, rval);
  }
  JS::RootedString valToStrRooted(cx, valToStr);
  errorString += JS_EncodeStringToUTF8(cx, valToStrRooted).get();
  PyErr_SetString(PyExc_TypeError, errorString.c_str());
  return NULL;
}