#include "include/PyType.hh"

#include "include/TypeEnum.hh"

#include <Python.h>

PyType::PyType(PyObject *object) {
  Py_XINCREF(object);
  pyObject = object;
}

PyObject *PyType::getPyObject() {
  return pyObject;
}

TYPE PyType::getReturnType() {
  return returnType;
}

PyType::~PyType() {
  Py_XDECREF(pyObject);
}