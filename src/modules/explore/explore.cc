#include "include/modules/explore/explore.hh"

#include "include/FuncType.hh"
#include "include/IntType.hh"
#include "include/ListType.hh"
#include "include/PyEvaluator.hh"
#include "include/pyTypeFactory.hh"
#include "include/StrType.hh"
#include "include/TupleType.hh"
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
ListType *factor_int(IntType *x) {
  ListType *list = new ListType();
  int n = x->getValue();

  for (int i = 1; i < sqrt(n); i++) {
    if (n % i == 0) {
      IntType *a = new IntType(i);
      IntType *b = new IntType(n/i);

      list->append(a);
      list->append(b);
    }
  }

  list->sort();

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

  return factor_int(input)->getPyObject();
}

static PyObject *pfactor(PyObject *self, PyObject *args) {
  PyEvaluator p = PyEvaluator();
  TupleType *arguments = new TupleType(args);

  PyType *result = p.eval("import math\ndef f(n):\n\treturn [x for x in range(1, n + 1) if n % x == 0]\n", "pfactor", arguments);

  if (result) {
    return result->getPyObject();
  }
  else {
    return NULL;
  }
}

static PyObject *run(PyObject *self, PyObject *args) {
  PyEvaluator p = PyEvaluator();

  StrType *input = new StrType(PyTuple_GetItem(args, 0));

  p.eval(input->getValue());

  Py_RETURN_NONE;
}

static PyMethodDef ExploreMethods[] = {
  {"output", output, METH_VARARGS, "Multivariatic function outputs"},
  {"factor", factor, METH_VARARGS, "Factor a python integer in C++"},
  {"pfactor", pfactor, METH_VARARGS, "Factor a python integer in C++ using python"},
  {"run", run, METH_VARARGS, "Run an arbirtrary python command in c++"},
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