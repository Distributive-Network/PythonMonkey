#include "include/BoolType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <Python.h>

#include <iostream>


BoolType::BoolType(PyObject *object) : PyType(object) {}

BoolType::BoolType(long n) : PyType(PyBool_FromLong(n)) {}

long BoolType::getValue() const {
  return PyLong_AS_LONG(pyObject);
}

void BoolType::print(std::ostream &os) const {
  os << this->getValue();
}