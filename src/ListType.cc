#include "include/ListType.hh"

#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"
#include "include/utilities.hh"

#include <Python.h>

#include <string>
#include <iostream>

ListType::ListType(PyObject *object) : PyType(object) {}

PyType *ListType::get(int index) const {
  return pyTypeFactory(PyList_GetItem(this->pyObject, index));
}

void ListType::set(int index, PyType *object) {
  PyList_SetItem(this->pyObject, index, object->getPyObject());
}

void ListType::append(PyType *value) {
  PyList_Append(this->pyObject, value->getPyObject());
}

void ListType::print(std::ostream &os) const {}