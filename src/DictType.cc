#include "include/DictType.hh"

#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"
#include "include/utilities.hh"

#include <Python.h>

#include <string>
#include <iostream>

DictType::DictType(PyObject *object) : PyType(object) {}

void DictType::set(PyType *key, PyType *value) {
  PyDict_SetItem(this->pyObject, key->getPyObject(), value->getPyObject());
}

PyType *DictType::get(PyType *key) const {
  PyObject *retrieved_object = PyDict_GetItem(this->pyObject, key->getPyObject());
  return retrieved_object != NULL ? pyTypeFactory(retrieved_object) : nullptr;
}

void DictType::print_helper(std::ostream &os, int depth) const {
  PyObject *keys = PyDict_Keys(this->pyObject);
  const Py_ssize_t keys_size = PyList_Size(keys);

  os << "{\n  ";
  for (int i = 0; i < keys_size; i++) {
    PyType *key = pyTypeFactory(PyList_GetItem(keys, i));
    PyType *value = this->get(key);

    if (instanceof<DictType>(value)) {
      DictType *casted_value = dynamic_cast<DictType *>(value);
      os << *key << ":";
      casted_value->print_helper(os, depth + 1);
    } else {
      os << std::string(depth * 2, ' ') << *key << ":" << *value;
    }
    if (i < keys_size - 1) {
      os << ",\n  ";
    }
  }
  os << std::endl << std::string(depth * 2, ' ') << "}";
}

void DictType::print(std::ostream &os) const {
  print_helper(os);
}