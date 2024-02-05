#include "include/TupleType.hh"

#include <Python.h>

#include <string>

TupleType::TupleType(PyObject *object) : PyType(object) {}