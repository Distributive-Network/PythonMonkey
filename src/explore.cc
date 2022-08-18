#include "include/explore.hh"

#include "include/FuncType.hh"
#include "include/IntType.hh"
#include "include/pyTypeFactory.hh"
#include "include/StrType.hh"
#include "include/utilities.hh"

#include <Python.h>

#include <math.h>

#include <iostream>

/**
 * @brief Factors an IntType
 *
 * @param x The IntType representation of the integer you want to factor
 * @return PyObject* a list which is not referenced by the python garbage collector
 */
PyObject *factor_int(IntType *x) {
  PyObject *list = PyList_New(0);
  int n = x->getValue();
  Py_XINCREF(list);

  for (int i = 1; i < sqrt(n); i++) {
    if (n % i == 0) {
      PyObject *f_1 = Py_BuildValue("i", i);
      PyObject *f_2 = Py_BuildValue("i", n / i);

      PyList_Append(list, f_1);
      PyList_Append(list, f_2);
    }
  }

  PyList_Sort(list);

  Py_XDECREF(list);
  return list;
}

static PyObject *output(PyObject *self, PyObject *args) {
  const int size = PyTuple_Size(args);
  for (int i = 0; i < size; i++) {
    PyType *item = pyTypeFactory(PyTuple_GET_ITEM(args, i));

    std::cout << *item << std::endl;
  }
  Py_RETURN_NONE;
}

static PyObject *factor(PyObject *self, PyObject *args) {
  IntType *input = new IntType(PyTuple_GetItem(args, 0));

  return factor_int(input);
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