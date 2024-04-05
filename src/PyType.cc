/**
 * @file PyType.cc
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Struct representing python types
 * @date 2022-07-27
 *
 * @copyright Copyright (c) 2022 Distributive Corp.
 *
 */

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <Python.h>

PyType::PyType() {}

PyType::PyType(PyObject *object) {
  Py_XINCREF(object);
  pyObject = object;
}

PyObject *PyType::getPyObject() {
  return pyObject;
}

PyType::~PyType() {
  Py_XDECREF(pyObject);
}