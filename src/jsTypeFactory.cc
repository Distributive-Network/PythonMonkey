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

#include <jsapi.h>

JS::Value jsTypeFactory(PyObject *object) {
  JS::Value returnType;

  if (PyBool_Check(object)) {
    returnType.setBoolean(PyLong_AsLong(object));
  }
  else if (PyLong_Check(object)) {
    returnType.setNumber(PyLong_AsLong(object));
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