#include "include/FuncType.hh"

#include "include/PyType.hh"

#include <Python.h>

#include <iostream>

FuncType::FuncType(PyObject *object) : PyType(object) {}

TYPE FuncType::getReturnType() {
  return TYPE::FUNC;
}
const char *FuncType::getValue() const {
  return PyUnicode_AsUTF8(PyObject_GetAttrString(pyObject, "__name__"));
}