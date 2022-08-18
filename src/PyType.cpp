#include <Python.h>
#include <string>
#include "../include/PyType.hpp"

PyType::PyType(PyObject *object) {
  Py_XINCREF(object);
  pyObject = object;
}

PyType::~PyType() {
  Py_XDECREF(pyObject);
}

PyObject *PyType::getPyObject() {
  return pyObject;
}

std::string PyType::getReturnType() {
  return returnType;
}