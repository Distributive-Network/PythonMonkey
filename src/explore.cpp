#include <iostream>
#include <Python.h>
#include "../include/PyTypeFactory.hpp"
#include "../include/IntType.hpp"
#include "../include/StrType.hpp"
#include "../include/FuncType.hpp"

static PyObject* output(PyObject* self, PyObject *args) {
  const int size = PyTuple_Size(args);
  for (int i = 0; i < size; i++) {
    PyType* item = PyTypeFactory(PyTuple_GET_ITEM(args, i));

    std::cout << *item;
  }
  Py_RETURN_NONE;
}

static PyMethodDef ExploreMethods[] = {
  {"output", output, METH_VARARGS, "Multivariatic function outputs"},
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