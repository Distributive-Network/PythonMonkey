#ifndef Bifrost_PyTuple_
#define Bifrost_PyTuple_

#include <Python.h>

class PyTuple {
private:

PyObject *tuple;

public:

PyTuple(PyObject *_tuple) : tuple(_tuple) {};

PyObject *get(int n);
Py_ssize_t getSize();

};

#endif