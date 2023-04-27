#include "include/PyType.hh"

#include "include/TypeEnum.hh"

#include <Python.h>

PyType::PyType() {}

PyType::PyType(PyObject *object) {
  Py_XINCREF(object);
  pyObject = object;
}

PyObject *PyType::getPyObject() {
  return pyObject;
}

PyType::~PyType() {
  Py_XDECREF(pyObject);
}