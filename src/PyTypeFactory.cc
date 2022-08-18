#include "include/PyTypeFactory.hh"

#include "include/DictType.hh"
#include "include/FuncType.hh"
#include "include/IntType.hh"
#include "include/PyType.hh"
#include "include/StrType.hh"

#include <Python.h>

PyType *PyTypeFactory(PyObject *object) {
  PyType *pyType;

  if (PyLong_Check(object))
    pyType = new IntType(object);
  else if (PyUnicode_Check(object))
    pyType = new StrType(object);
  else if (PyFunction_Check(object))
    pyType = new FuncType(object);
  else if (PyDict_Check(object))
    pyType = new DictType(object);
  else
    return nullptr;

  return pyType;
}