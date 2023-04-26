#include "include/PyType.hh"

#include "include/TypeEnum.hh"

#include <Python.h>

PyType::PyType() : returnType(TYPE::DEFAULT) {}

PyType::PyType(PyObject *object) : returnType(TYPE::DEFAULT) {
  Py_XINCREF(object);
  pyObject = object;
}

PyObject *PyType::getPyObject() {
  return pyObject;
}

PyType::~PyType() {
  Py_XDECREF(pyObject);
}