/**
 * @file TupleType.cc
 * @author Giovanni Tedesco
 * @brief Implementation for the methods of the Tuple Type struct
 * @version 0.1
 * @date 2022-08-19
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "include/TupleType.hh"

#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"
#include "include/utilities.hh"

#include <Python.h>

#include <string>
#include <iostream>

TupleType::TupleType(PyObject *object) : PyType(object) {}

PyType *TupleType::get(int index) const {
  return pyTypeFactory(PyTuple_GetItem(this->pyObject, index));
}

int TupleType::len() const {
  return PyTuple_Size(this->pyObject);
}

void TupleType::print_helper(std::ostream &os, int depth) const {
  int size = this->len();
  os << "(\n  ";
  for (int i = 0; i < size; i++) {
    PyType *value = this->get(i);

    if (instanceof<TupleType>(value)) {
      TupleType *casted_value = dynamic_cast<TupleType *>(value);
      casted_value->print_helper(os, depth + 1);
    } else {
      os << std::string(depth * 2, ' ') << *value;
    }
    if (i < size - 1) {
      os << ",\n  ";
    }
  }
  os << std::endl << std::string(depth * 2, ' ') << ")";
}

void TupleType::print(std::ostream &os) const {
  print_helper(os);
}