/**
 * @file jsTypeFactory.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief
 * @version 0.1
 * @date 2023-02-15
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "include/jsTypeFactory.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/PyType.hh"
#include "include/FuncType.hh"
#include "include/pyTypeFactory.hh"
#include "include/StrType.hh"
#include "include/IntType.hh"

#include <jsapi.h>
#include <jsfriendapi.h>

#include <Python.h>

#define HIGH_SURROGATE_START 0xD800
#define LOW_SURROGATE_START 0xDC00
#define LOW_SURROGATE_END 0xDFFF
#define BMP_END 0x10000

struct PythonExternalString : public JSExternalStringCallbacks {
  void finalize(char16_t *chars) const override {}
  size_t sizeOfBuffer(const char16_t *chars, mozilla::MallocSizeOf mallocSizeOf) const override {
    return 0;
  }
};

static constexpr PythonExternalString PythonExternalStringCallbacks;

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
  JS::RootedValue returnType(cx);

  if (PyBool_Check(object)) {
    returnType.setBoolean(PyLong_AsLong(object));
  }
  else if (PyLong_Check(object)) {
    if (PyObject_IsInstance(object, PythonMonkey_BigInt)) { // pm.bigint is a subclass of the builtin int type
      JS::BigInt *bigint = IntType(object).toJsBigInt(cx);
      returnType.setBigInt(bigint);
    } else if (_PyLong_NumBits(object) <= 53) { // num <= JS Number.MAX_SAFE_INTEGER, the mantissa of a float64 is 53 bits (with 52 explicitly stored and the highest bit always being 1)
      uint64_t num = PyLong_AsLongLong(object);
      returnType.setNumber(num);
    } else {
      PyErr_SetString(PyExc_OverflowError, "Absolute value of the integer exceeds JS Number.MAX_SAFE_INTEGER. Use pythonmonkey.bigint instead.");
    }
  }
  else if (PyFloat_Check(object)) {
    returnType.setNumber(PyFloat_AsDouble(object));
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
        JSString *str = JS_NewExternalString(cx, (char16_t *)PyUnicode_2BYTE_DATA(object), PyUnicode_GET_LENGTH(object), &PythonExternalStringCallbacks);
        returnType.setString(str);
        break;
      }
    case (PyUnicode_1BYTE_KIND): {

        JSString *str = JS_NewExternalString(cx, (char16_t *)PyUnicode_1BYTE_DATA(object), PyUnicode_GET_LENGTH(object), &PythonExternalStringCallbacks);
        /* @TODO (Caleb Aikens) this is a hack to set the JSString::LATIN1_CHARS_BIT, because there isnt an API for latin1 JSExternalStrings.
         * Ideally we submit a patch to Spidermonkey to make this part of their API with the following signature:
         * JS_NewExternalString(JSContext *cx, const char *chars, size_t length, const JSExternalStringCallbacks *callbacks)
         */
        *(std::atomic<unsigned long> *)str |= 512;
        returnType.setString(str);
        break;
      }
    }
    memoizePyTypeAndGCThing(new StrType(object), returnType);
  }
  else if (PyFunction_Check(object)) {
    /*
     * import inspect
     * args = (inspect.getfullargspec(object)).args
     */
    PyObject *const inspect = PyImport_Import(PyUnicode_DecodeFSDefault("inspect"));
    PyObject *const getfullargspec = PyObject_GetAttrString(inspect, "getfullargspec");
    PyObject *const getfullargspecArgs = PyTuple_New(1);
    PyTuple_SetItem(getfullargspecArgs, 0, object);
    PyObject *const argspec = PyObject_CallObject(getfullargspec, getfullargspecArgs);
    PyObject *const args = PyObject_GetAttrString(argspec, "args");

    JSFunction *jsFunc = js::NewFunctionWithReserved(cx, callPyFunc, PyList_Size(args), 0, NULL);
    JSObject *jsFuncObject = JS_GetFunctionObject(jsFunc);

    // We put the address of the PyObject in the JSFunction's 0th private slot so we can access it later
    js::SetFunctionNativeReserved(jsFuncObject, 0, JS::PrivateValue((void *)object));
    returnType.setObject(*jsFuncObject);
    memoizePyTypeAndGCThing(new FuncType(object), returnType);
  }
  else if (object == Py_None) {
    returnType.setUndefined();
  }
  else if (object == PythonMonkey_Null) {
    returnType.setNull();
  }
  else {
    PyErr_SetString(PyExc_TypeError, "Python types other than bool, int, pythonmonkey.bigint, float, str, None, and our custom Null type are not supported by pythonmonkey yet.");
  }
  return returnType;

}

bool callPyFunc(JSContext *cx, unsigned int argc, JS::Value *vp) {
  JS::CallArgs callargs = JS::CallArgsFromVp(argc, vp);

  // get the python function from the 0th reserved slot
  JS::Value pyFuncVal = js::GetFunctionNativeReserved(&(callargs.callee()), 0);
  PyObject *pyFunc = (PyObject *)(pyFuncVal.toPrivate());

  JS::RootedObject thisv(cx);
  JS_ValueToObject(cx, callargs.thisv(), &thisv);

  if (argc == 0) {
    PyObject *pyRval = PyObject_CallNoArgs(pyFunc);
    // @TODO (Caleb Aikens) need to check for python exceptions here
    callargs.rval().set(jsTypeFactory(cx, pyRval));
    return true;
  }

  // populate python args tuple
  PyObject *pyArgs = PyTuple_New(argc);
  for (size_t i = 0; i < argc; i++) {
    JS::RootedValue jsArg = JS::RootedValue(cx, callargs[i]);
    PyType *pyArg = (pyTypeFactory(cx, &thisv, &jsArg));
    PyTuple_SetItem(pyArgs, i, pyArg->getPyObject());
  }

  PyObject *pyRval = PyObject_Call(pyFunc, pyArgs, NULL);
  // @TODO (Caleb Aikens) need to check for python exceptions here
  callargs.rval().set(jsTypeFactory(cx, pyRval));

  return true;
}