#include "include/PyTuple.hh"

#include <Python.h>

PyObject *PyTuple::get(int n) {
  return PyTuple_GetItem(tuple, n);
}

Py_ssize_t PyTuple::getSize() {
  return PyTuple_GET_SIZE(tuple);
}