#ifndef Bifrost_PyTuple_
#define Bifrost_PyTuple_

#include <Python.h>

class PyTuple {
public:
PyTuple(PyObject *_tuple) : tuple(_tuple) {};
PyObject *get(int n);
Py_ssize_t getSize();

private:
PyObject *tuple;
};

#endif