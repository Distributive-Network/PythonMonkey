#ifndef PYTUPLE_HPP
#define PYTUPLE_HPP

#include <Python.h>

class PyTuple {
private:

    PyObject* tuple;

public:

    PyTuple(PyObject* _tuple) : tuple(_tuple) {};

    PyObject* get(int n);
    Py_ssize_t getSize();

};

#endif