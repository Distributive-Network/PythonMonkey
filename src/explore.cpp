#include <iostream>
#include <Python.h>
#include "../include/PyTypeFactory.hpp"
#include "../include/IntType.hpp"
#include "../include/StrType.hpp"
#include "../include/FuncType.hpp"
#include "include/utilities.hpp"

static PyObject* output(PyObject* self, PyObject *args) {
  const int size = PyTuple_Size(args);
  for (int i = 0; i < size; i++) {
    PyType* item = PyTypeFactory(PyTuple_GET_ITEM(args, i));

    std::cout << *item;
  }
  Py_RETURN_NONE;
}

static PyObject* factor(PyObject* self, PyObject* args) {

  PyType* item = PyTypeFactory(PyTuple_GetItem(args, 0));

  if(instanceof<IntType>(item)) {
    IntType* casted_int = dynamic_cast<IntType*>(item);
    return casted_int->factor();
  } else {
    PyErr_SetNone(PyExc_TypeError);
    PyErr_Occurred();
    Py_RETURN_NONE;
  }
}

static PyMethodDef ExploreMethods[] = {
  {"output", output, METH_VARARGS, "Multivariatic function outputs"},
  {"factor", factor, METH_VARARGS, "Factor a python integer in C++"},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef explore =
{
  PyModuleDef_HEAD_INIT,
  "explore",     /* name of module */
  "",          /* module documentation, may be NULL */
  -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
  ExploreMethods
};

PyMODINIT_FUNC PyInit_explore(void)
{
  return PyModule_Create(&explore);
}