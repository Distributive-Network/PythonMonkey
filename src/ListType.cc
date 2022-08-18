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

int ListType::len() const {
  return PyList_Size(this->pyObject);
}

void ListType::print_helper(std::ostream &os, int depth) const {
  int size = this->len();
  os << "[\n  ";
  for (int i = 0; i < size; i++) {
    PyType *value = this->get(i);

    if (instanceof<ListType>(value)) {
      ListType *casted_value = dynamic_cast<ListType *>(value);
      casted_value->print_helper(os, depth + 1);
    } else {
      os << std::string(depth * 2, ' ') << *value;
    }
    if (i < size - 1) {
      os << ",\n  ";
    }
  }
  os << std::endl << std::string(depth * 2, ' ') << "]";
}

void ListType::print(std::ostream &os) const {
  print_helper(os);
}