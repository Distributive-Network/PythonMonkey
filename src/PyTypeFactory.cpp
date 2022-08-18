#include <Python.h>
#include "../include/PyTypeFactory.hpp"
#include "../include/IntType.hpp"
#include "../include/StrType.hpp"
#include "../include/FuncType.hpp"

PyType *PyTypeFactory(PyObject *object) {
  PyType *pyType;

  if (PyLong_Check(object))
    pyType = new IntType(object);
  else if (PyUnicode_Check(object))
    pyType = new StrType(object);
  else if (PyFunction_Check(object))
    pyType = new FuncType(object);
  else
    return nullptr;

  return pyType;
}
