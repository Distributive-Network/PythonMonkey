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
#include "include/StrType.hh"

#include <jsapi.h>

#include <Python.h>

#define HIGH_SURROGATE_START 0xD800
#define HIGH_SURROGATE_END 0xDBFF
#define LOW_SURROGATE_START 0xDC00
#define LOW_SURROGATE_END 0xDFFF
#define BMP_END 0x10000

size_t UCS4ToUTF16(const uint32_t *chars, size_t length, uint16_t *outStr) {
  uint16_t utf16String[length*2];
  size_t utf16Length = 0;

  for (size_t i = 0; i < length; i++) {
    if (chars[i] < HIGH_SURROGATE_START || (chars[i] > LOW_SURROGATE_END && chars[i] < BMP_END)) {
      utf16String[utf16Length] = uint16_t(chars[i]);
      utf16Length += 1;
    }
    else {
      utf16String[utf16Length] = uint16_t(((0b1111'1111'1100'0000'0000 & (chars[i] - BMP_END)) >> 10) + HIGH_SURROGATE_START);
      utf16String[utf16Length + 1] = uint16_t(((0b0000'0000'0011'1111'1111 & (chars[i] - BMP_END)) >> 00) +  LOW_SURROGATE_START);
      utf16Length += 2;
    }
  }
  outStr = utf16String;
  return utf16Length;
}

JS::Value jsTypeFactory(JSContext *cx, PyObject *object) {
  JS::RootedValue returnType(cx);

  if (PyBool_Check(object)) {
    returnType.setBoolean(PyLong_AsLong(object));
  }
  else if (PyLong_Check(object)) {
    returnType.setNumber(PyLong_AsLong(object));
  }
  else if (PyFloat_Check(object)) {
    returnType.setNumber(PyFloat_AsDouble(object));
  }
  else if (PyUnicode_Check(object)) {
    switch (PyUnicode_KIND(object)) {
    case (PyUnicode_4BYTE_KIND): {
        uint32_t *u32Chars = PyUnicode_4BYTE_DATA(object);
        uint16_t *u16Chars;
        size_t u16Length = UCS4ToUTF16(u32Chars, PyUnicode_GET_LENGTH(object), u16Chars);
        JSString *str = JS_NewUCStringCopyN(cx, (char16_t *)u16Chars, u16Length);
        returnType.setString(str);
        break;
      }
    case (PyUnicode_2BYTE_KIND): {
        JS::UniqueTwoByteChars chars((char16_t *)PyUnicode_2BYTE_DATA(object));
        JSString *str = JS_NewUCString(cx, std::move(chars), PyUnicode_GET_LENGTH(object));
        returnType.setString(str);
        break;
      }
    case (PyUnicode_1BYTE_KIND): {
        JS::UniqueLatin1Chars chars(PyUnicode_1BYTE_DATA(object));
        JSString *str = JS_NewLatin1String(cx, std::move(chars), PyUnicode_GET_LENGTH(object));
        returnType.setString(str);
        break;
      }
    }
    memoizePyTypeAndGCThing(new StrType(object), returnType);
  }
  else if (object == Py_None) {
    returnType.setUndefined();
  }
  else if (object == PythonMonkey_Null) {
    returnType.setNull();
  }
  else {
    PyErr_SetString(PyExc_TypeError, "Python types other than bool, int, float, None, and our custom Null type are not supported by pythonmonkey yet.");
  }
  return returnType;

}