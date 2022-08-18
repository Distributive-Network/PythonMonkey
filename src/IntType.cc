#include "include/IntType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <Python.h>

#include <iostream>


IntType::IntType(PyObject *object) : PyType(object) {}

IntType::IntType(long n) : PyType(Py_BuildValue("i", n)) {}

void IntType::print(std::ostream &os) const {
  os << this->getValue();
}

long IntType::getValue() const {
  return PyLong_AS_LONG(pyObject);
}