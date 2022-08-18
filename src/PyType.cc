#include "include/PyType.hh"

#include "include/TypeEnum.hh"

#include <Python.h>


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

TYPE PyType::getReturnType() {
  return returnType;
}