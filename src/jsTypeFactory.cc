/**
 * @file jsTypeFactory.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief
 * @date 2023-02-15
 *
 * @copyright 2023-2024 Distributive Corp.
 *
 */

#include "include/jsTypeFactory.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/JSFunctionProxy.hh"
#include "include/JSMethodProxy.hh"
#include "include/JSObjectProxy.hh"
#include "include/JSArrayProxy.hh"
#include "include/PyDictProxyHandler.hh"
#include "include/JSStringProxy.hh"
#include "include/PyListProxyHandler.hh"
#include "include/PyObjectProxyHandler.hh"
#include "include/PyIterableProxyHandler.hh"
#include "include/pyTypeFactory.hh"
#include "include/IntType.hh"
#include "include/PromiseType.hh"
#include "include/DateType.hh"
#include "include/ExceptionType.hh"
#include "include/BufferType.hh"
#include "include/setSpiderMonkeyException.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Equality.h>
#include <js/Proxy.h>
#include <js/Array.h>

#include <Python.h>
#include <datetime.h>

#include <unordered_map>

#define HIGH_SURROGATE_START 0xD800
#define LOW_SURROGATE_START 0xDC00
#define LOW_SURROGATE_END 0xDFFF
#define BMP_END 0x10000

static PyDictProxyHandler pyDictProxyHandler;
static PyObjectProxyHandler pyObjectProxyHandler;
static PyListProxyHandler pyListProxyHandler;
static PyIterableProxyHandler pyIterableProxyHandler;

std::unordered_map<PyObject *, size_t> externalStringObjToRefCountMap;// a map of python string objects to the number of JSExternalStrings that depend on it, used when finalizing JSExternalStrings

PyObject *PythonExternalString::getPyString(const char16_t *chars)
{
  for (auto it: externalStringObjToRefCountMap) {
    if (PyUnicode_DATA(it.first) == (void *)chars) { // PyUnicode_<2/1>BYTE_DATA are just type casts of PyUnicode_DATA
      return it.first;
    }
  }

  return NULL; // this shouldn't be reachable
}

PyObject *PythonExternalString::getPyString(const JS::Latin1Char *chars)
{
  
  return PythonExternalString::getPyString((const char16_t *)chars);
}

void PythonExternalString::finalize(char16_t *chars) const
{
  // We cannot call Py_DECREF here when shutting down as the thread state is gone.
  // Then, when shutting down, there is only on reference left, and we don't need
  // to free the object since the entire process memory is being released.
  if (_Py_IsFinalizing()) { return; }

  for (auto it = externalStringObjToRefCountMap.cbegin(), next_it = it; it != externalStringObjToRefCountMap.cend(); it = next_it) {
    next_it++;
    if (PyUnicode_DATA(it->first) == (void *)chars) {
      Py_DECREF(it->first);
      externalStringObjToRefCountMap[it->first] = externalStringObjToRefCountMap[it->first] - 1;

      if (externalStringObjToRefCountMap[it->first] == 0) {
        externalStringObjToRefCountMap.erase(it);
      }
    }
  }
}

void PythonExternalString::finalize(JS::Latin1Char *chars) const
{
  PythonExternalString::finalize((char16_t *)chars);
}

size_t PythonExternalString::sizeOfBuffer(const char16_t *chars, mozilla::MallocSizeOf mallocSizeOf) const
{
  for (auto it: externalStringObjToRefCountMap) {
    if (PyUnicode_DATA(it.first) == (void *)chars) {
      return PyUnicode_GetLength(it.first);
    }
  }

  return 0; // // this shouldn't be reachable
}

size_t PythonExternalString::sizeOfBuffer(const JS::Latin1Char *chars, mozilla::MallocSizeOf mallocSizeOf) const
{
  return PythonExternalString::sizeOfBuffer((const char16_t *)chars, mallocSizeOf);
}

PythonExternalString PythonExternalStringCallbacks = {};

size_t UCS4ToUTF16(const uint32_t *chars, size_t length, uint16_t **outStr) {
  uint16_t *utf16String = (uint16_t *)malloc(sizeof(uint16_t) * length*2);
  size_t utf16Length = 0;

  for (size_t i = 0; i < length; i++) {
    if (chars[i] < HIGH_SURROGATE_START || (chars[i] > LOW_SURROGATE_END && chars[i] < BMP_END)) {
      utf16String[utf16Length] = uint16_t(chars[i]);
      utf16Length += 1;
    }
    else {
      /* *INDENT-OFF* */
      utf16String[utf16Length]      = uint16_t(((0b1111'1111'1100'0000'0000 & (chars[i] - BMP_END)) >> 10) + HIGH_SURROGATE_START);
      utf16String[utf16Length + 1]  = uint16_t(((0b0000'0000'0011'1111'1111 & (chars[i] - BMP_END)) >> 00) +  LOW_SURROGATE_START);
      utf16Length += 2;
      /* *INDENT-ON* */
    }
  }
  *outStr = utf16String;
  return utf16Length;
}

JS::Value jsTypeFactory(JSContext *cx, PyObject *object) {
  if (!PyDateTimeAPI) { PyDateTime_IMPORT; } // for PyDateTime_Check

  JS::RootedValue returnType(cx);

  if (PyBool_Check(object)) {
    returnType.setBoolean(PyLong_AsLong(object));
  }
  else if (PyLong_Check(object)) {
    if (PyObject_IsInstance(object, getPythonMonkeyBigInt())) { // pm.bigint is a subclass of the builtin int type
      JS::BigInt *bigint = IntType::toJsBigInt(cx, object);
      returnType.setBigInt(bigint);
    } else if (_PyLong_NumBits(object) <= 53) { // num <= JS Number.MAX_SAFE_INTEGER, the mantissa of a float64 is 53 bits (with 52 explicitly stored and the highest bit always being 1)
      int64_t num = PyLong_AsLongLong(object);
      returnType.setNumber(num);
    } else {
      PyErr_SetString(PyExc_OverflowError, "Absolute value of the integer exceeds JS Number.MAX_SAFE_INTEGER. Use pythonmonkey.bigint instead.");
    }
  }
  else if (PyFloat_Check(object)) {
    returnType.setNumber(PyFloat_AsDouble(object));
  }
  else if (PyObject_TypeCheck(object, &JSStringProxyType)) {
    returnType.setString(((JSStringProxy *)object)->jsString->toString());
  }
  else if (PyUnicode_Check(object)) {
    switch (PyUnicode_KIND(object)) {
    case (PyUnicode_4BYTE_KIND): {
        uint32_t *u32Chars = PyUnicode_4BYTE_DATA(object);
        uint16_t *u16Chars;
        size_t u16Length = UCS4ToUTF16(u32Chars, PyUnicode_GET_LENGTH(object), &u16Chars);
        JSString *str = JS_NewUCStringCopyN(cx, (char16_t *)u16Chars, u16Length);
        free(u16Chars);
        returnType.setString(str);
        break;
      }
    case (PyUnicode_2BYTE_KIND): {
        externalStringObjToRefCountMap[object] = externalStringObjToRefCountMap[object] + 1;
        Py_INCREF(object);
        JSString *str = JS_NewExternalUCString(cx, (char16_t *)PyUnicode_2BYTE_DATA(object), PyUnicode_GET_LENGTH(object), &PythonExternalStringCallbacks);
        returnType.setString(str);
        break;
      }
    case (PyUnicode_1BYTE_KIND): {
        externalStringObjToRefCountMap[object] = externalStringObjToRefCountMap[object] + 1;
        Py_INCREF(object);
        JSString *str = JS_NewExternalStringLatin1(cx, (JS::Latin1Char *)PyUnicode_1BYTE_DATA(object), PyUnicode_GET_LENGTH(object), &PythonExternalStringCallbacks);
        // JSExternalString can now be properly treated as either one-byte or two-byte strings when GCed
        // see https://hg.mozilla.org/releases/mozilla-esr128/file/tip/js/src/vm/StringType-inl.h#l785
        returnType.setString(str);
        break;
      }
    }
  }
  else if (PyMethod_Check(object) || PyFunction_Check(object) || PyCFunction_Check(object)) {
    // can't determine number of arguments for PyCFunctions, so just assume potentially unbounded
    uint16_t nargs = 0;
    if (PyFunction_Check(object)) {
      PyCodeObject *bytecode = (PyCodeObject *)PyFunction_GetCode(object); // borrowed reference
      nargs = bytecode->co_argcount;
    }

    JSFunction *jsFunc = js::NewFunctionWithReserved(cx, callPyFunc, nargs, 0, NULL);
    JS::RootedObject jsFuncObject(cx, JS_GetFunctionObject(jsFunc));
    // We put the address of the PyObject in the JSFunction's 0th private slot so we can access it later
    js::SetFunctionNativeReserved(jsFuncObject, 0, JS::PrivateValue((void *)object));
    returnType.setObject(*jsFuncObject);
    Py_INCREF(object); // otherwise the python function object would be double-freed on GC in Python 3.11+

    // add function to jsFunctionRegistry, to DECREF the PyObject when the JSFunction is finalized
    JS::RootedValueArray<2> registerArgs(GLOBAL_CX);
    registerArgs[0].setObject(*jsFuncObject);
    registerArgs[1].setPrivate(object);
    JS::RootedValue ignoredOutVal(GLOBAL_CX);
    JS::RootedObject registry(GLOBAL_CX, jsFunctionRegistry);
    if (!JS_CallFunctionName(GLOBAL_CX, registry, "register", registerArgs, &ignoredOutVal)) {
      setSpiderMonkeyException(GLOBAL_CX);
      return returnType;
    }
  }
  else if (PyExceptionInstance_Check(object)) {
    JSObject *error = ExceptionType::toJsError(cx, object, nullptr);
    if (error) {
      returnType.setObject(*error);
    }
    else {
      returnType.setUndefined();
    }
  }
  else if (PyDateTime_Check(object)) {
    JSObject *dateObj = DateType::toJsDate(cx, object);
    returnType.setObject(*dateObj);
  }
  else if (PyObject_CheckBuffer(object)) {
    JSObject *typedArray = BufferType::toJsTypedArray(cx, object); // may return null
    returnType.setObjectOrNull(typedArray);
  }
  else if (PyObject_TypeCheck(object, &JSObjectProxyType)) {
    returnType.setObject(**((JSObjectProxy *)object)->jsObject);
  }
  else if (PyObject_TypeCheck(object, &JSMethodProxyType)) {
    JS::RootedObject func(cx, *((JSMethodProxy *)object)->jsFunc);
    PyObject *self = ((JSMethodProxy *)object)->self;

    JS::Rooted<JS::ValueArray<1>> args(cx);
    args[0].set(jsTypeFactory(cx, self));
    JS::Rooted<JS::Value> boundFunction(cx);
    if (!JS_CallFunctionName(cx, func, "bind", args, &boundFunction)) {
      setSpiderMonkeyException(GLOBAL_CX);
      return returnType;
    }
    returnType.set(boundFunction);
    // add function to jsFunctionRegistry, to DECREF the PyObject when the JSFunction is finalized
    JS::RootedValueArray<2> registerArgs(GLOBAL_CX);
    registerArgs[0].set(boundFunction);
    registerArgs[1].setPrivate(object);
    JS::RootedValue ignoredOutVal(GLOBAL_CX);
    JS::RootedObject registry(GLOBAL_CX, jsFunctionRegistry);
    if (!JS_CallFunctionName(GLOBAL_CX, registry, "register", registerArgs, &ignoredOutVal)) {
      setSpiderMonkeyException(GLOBAL_CX);
      return returnType;
    }

    Py_INCREF(object);
  }
  else if (PyObject_TypeCheck(object, &JSFunctionProxyType)) {
    returnType.setObject(**((JSFunctionProxy *)object)->jsFunc);
  }
  else if (PyObject_TypeCheck(object, &JSArrayProxyType)) {
    returnType.setObject(**((JSArrayProxy *)object)->jsArray);
  }
  else if (PyDict_Check(object) || PyList_Check(object)) {
    JS::RootedValue v(cx);
    JSObject *proxy;
    if (PyList_Check(object)) {
      JS::RootedObject arrayPrototype(cx);
      JS_GetClassPrototype(cx, JSProto_Array, &arrayPrototype); // so that instanceof will work, not that prototype methods will
      proxy = js::NewProxyObject(cx, &pyListProxyHandler, v, arrayPrototype.get());
    } else {
      JS::RootedObject objectPrototype(cx);
      JS_GetClassPrototype(cx, JSProto_Object, &objectPrototype); // so that instanceof will work, not that prototype methods will
      proxy = js::NewProxyObject(cx, &pyDictProxyHandler, v, objectPrototype.get());
    }
    Py_INCREF(object);
    JS::SetReservedSlot(proxy, PyObjectSlot, JS::PrivateValue(object));
    returnType.setObject(*proxy);
  }
  else if (object == Py_None) {
    returnType.setUndefined();
  }
  else if (object == getPythonMonkeyNull()) {
    returnType.setNull();
  }
  else if (PythonAwaitable_Check(object)) {
    returnType.setObjectOrNull(PromiseType::toJsPromise(cx, object));
  }
  else if (PyIter_Check(object)) {
    JS::RootedValue v(cx);
    JS::RootedObject objectPrototype(cx);
    JS_GetClassPrototype(cx, JSProto_Object, &objectPrototype); // so that instanceof will work, not that prototype methods will
    JSObject *proxy = js::NewProxyObject(cx, &pyIterableProxyHandler, v, objectPrototype.get());
    PyObject *iterable = PyObject_GetIter(object);
    Py_INCREF(iterable);
    JS::SetReservedSlot(proxy, PyObjectSlot, JS::PrivateValue(iterable));
    returnType.setObject(*proxy);
  }
  else {
    JS::RootedValue v(cx);
    JS::RootedObject objectPrototype(cx);
    JS_GetClassPrototype(cx, JSProto_Object, &objectPrototype); // so that instanceof will work, not that prototype methods will
    JSObject *proxy = js::NewProxyObject(cx, &pyObjectProxyHandler, v, objectPrototype.get());
    Py_INCREF(object);
    JS::SetReservedSlot(proxy, PyObjectSlot, JS::PrivateValue(object));
    returnType.setObject(*proxy);
  }
  return returnType;
}

JS::Value jsTypeFactorySafe(JSContext *cx, PyObject *object) {
  JS::Value v = jsTypeFactory(cx, object);
  if (PyErr_Occurred()) {
    // Convert the Python error to a warning
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback); // also clears Python's error stack
    PyObject *msg = PyObject_Str(value);
    PyErr_WarnEx(PyExc_RuntimeWarning, PyUnicode_AsUTF8(msg), 1);
    Py_DECREF(msg);
    Py_XDECREF(type); Py_XDECREF(value); Py_XDECREF(traceback);
    // Return JS `null` on error
    v.setNull();
  }
  return v;
}

bool setPyException(JSContext *cx) {
  // Python `exit` and `sys.exit` only raise a SystemExit exception to end the program
  // We definitely don't want to catch it in JS
  if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
    return false;
  }

  PyObject *type, *value, *traceback;
  PyErr_Fetch(&type, &value, &traceback);

  JSObject *jsException = ExceptionType::toJsError(cx, value, traceback);

  Py_XDECREF(type);
  Py_XDECREF(value);
  Py_XDECREF(traceback);

  if (jsException) {
    JS::RootedValue jsExceptionValue(cx, JS::ObjectValue(*jsException));
    JS_SetPendingException(cx, jsExceptionValue);
  }
  return true;
}

bool callPyFunc(JSContext *cx, unsigned int argc, JS::Value *vp) {
  JS::CallArgs callargs = JS::CallArgsFromVp(argc, vp);

  // get the python function from the 0th reserved slot
  PyObject *pyFunc = (PyObject *)js::GetFunctionNativeReserved(&(callargs.callee()), 0).toPrivate();
  Py_INCREF(pyFunc);
  PyObject *pyRval = NULL;
  PyObject *pyArgs = NULL;
  Py_ssize_t nNormalArgs = 0;   // number of positional non-default arguments
  Py_ssize_t nDefaultArgs = 0;  // number of positional default arguments
  bool varargs = false;
  bool unknownNargs = false;

  if (PyCFunction_Check(pyFunc)) {
    const int funcFlags = ((PyCFunctionObject *)pyFunc)->m_ml->ml_flags;
    if (funcFlags & METH_NOARGS) { // 0 arguments
      nNormalArgs = 0;
    }
    else if (funcFlags & METH_O) { // 1 argument
      nNormalArgs = 1;
    }
    else { // unknown number of arguments
      nNormalArgs = 0;
      unknownNargs = true;
      varargs = true;
    }
  }
  else {
    nNormalArgs = 1;
    PyObject *f = pyFunc;
    if (PyMethod_Check(pyFunc)) {
      f = PyMethod_Function(pyFunc); // borrowed reference
      nNormalArgs -= 1; // don't include the implicit `self` of the method as an argument
    }
    PyCodeObject *bytecode = (PyCodeObject *)PyFunction_GetCode(f); // borrowed reference
    PyObject *defaults = PyFunction_GetDefaults(f); // borrowed reference
    nDefaultArgs = defaults ? PyTuple_Size(defaults) : 0;
    nNormalArgs += bytecode->co_argcount - nDefaultArgs - 1;
    if (bytecode->co_flags & CO_VARARGS) {
      varargs = true;
    }
  }

  // use faster calling if no arguments are needed
  if (((nNormalArgs + nDefaultArgs) == 0 && !varargs)) {
    #if PY_VERSION_HEX >= 0x03090000
    pyRval = PyObject_CallNoArgs(pyFunc);
    #else
    pyRval = _PyObject_CallNoArg(pyFunc); // in Python 3.8, the API is only available under the name with a leading underscore
    #endif
    if (PyErr_Occurred() && setPyException(cx)) { // Check if an exception has already been set in Python error stack
      goto failure;
    }
    goto success;
  }

  // populate python args tuple
  Py_ssize_t argTupleLength;
  if (unknownNargs) { // pass all passed arguments
    argTupleLength = callargs.length();
  }
  else if (varargs) { // if passed arguments is less than number of non-default positionals, rest will be set to `None`
    argTupleLength = std::max((Py_ssize_t)callargs.length(), nNormalArgs);
  }
  else if (nNormalArgs > callargs.length()) { // if passed arguments is less than number of non-default positionals, rest will be set to `None`
    argTupleLength = nNormalArgs;
  }
  else { // passed arguments greater than non-default positionals, so we may be replacing default positional arguments
    argTupleLength = std::min((Py_ssize_t)callargs.length(), nNormalArgs+nDefaultArgs);
  }
  pyArgs = PyTuple_New(argTupleLength);

  for (size_t i = 0; i < callargs.length() && i < argTupleLength; i++) {
    JS::RootedValue jsArg(cx, callargs[i]);
    PyObject *pyArgObj = pyTypeFactory(cx, jsArg);
    if (!pyArgObj) return false; // error occurred
    PyTuple_SetItem(pyArgs, i, pyArgObj);
  }

  // set unspecified args to None, to match JS behaviour of setting unspecified args to undefined
  for (Py_ssize_t i = callargs.length(); i < argTupleLength; i++) {
    PyTuple_SetItem(pyArgs, i, Py_None);
  }

  pyRval = PyObject_Call(pyFunc, pyArgs, NULL);
  if (PyErr_Occurred() && setPyException(cx)) {
    goto failure;
  }
  goto success;

success:
  if (pyRval) { // can be NULL if SystemExit was raised
    callargs.rval().set(jsTypeFactory(cx, pyRval));
    Py_DECREF(pyRval);
  }
  Py_DECREF(pyFunc);
  Py_XDECREF(pyArgs);
  return true;

failure:
  Py_XDECREF(pyRval);
  Py_DECREF(pyFunc);
  Py_XDECREF(pyArgs);
  return false;
}