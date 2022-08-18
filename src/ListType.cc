#include "include/ListType.hh"

#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"
#include "include/utilities.hh"

#include <Python.h>

#include <string>
#include <iostream>

ListType::ListType(PyObject *object) : PyType(object) {}

PyType *ListType::get(int index) const {
  return nullptr;
}
void ListType::set(int index, PyType *object) {}

void ListType::print(std::ostream &os) const {}