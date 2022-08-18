#include "../include/FuncType.hpp"
#include <Python.h>

FuncType::FuncType(PyObject *object) : PyType(object) {}

void FuncType::print(std::ostream &os) const {
  os << this->getValue();
}

const char *FuncType::getValue() const {
  return PyUnicode_AsUTF8(PyObject_GetAttrString(pyObject, "__name__"));
}