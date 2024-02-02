#include "include/TupleType.hh"
#include "include/PyType.hh"

#include <Python.h>

#include <string>

TupleType::TupleType(PyObject *object) : PyType(object) {}