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
#include "include/IntType.hh"

#include <jsapi.h>

JS::Value jsTypeFactory(JSContext *cx, PyObject *object) {
  JS::Value returnType;

  if (PyBool_Check(object)) {
    returnType.setBoolean(PyLong_AsLong(object));
  }
  else if (PyLong_Check(object)) {
    if (PyObject_IsInstance(object, PythonMonkey_BigInt)) { // pm.bigint is a subclass of the builtin int type
      auto bigint = IntType(object).toJsBigInt(cx);
      returnType.setBigInt(bigint);
    } else {
      long long num = PyLong_AsLongLong(object);
      if (JS::Value::isNumberRepresentable(num)) { // TODO: refactor using _PyLong_NumBits ?
        returnType.setNumber(num);
      } else {
        PyErr_SetString(PyExc_TypeError, "Integer exceeds Number.MAX_SAFE_INTEGER. Use pythonmonkey.bigint instead.");
      }
    }
  }
  else if (PyFloat_Check(object)) {
    returnType.setNumber(PyFloat_AsDouble(object));
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