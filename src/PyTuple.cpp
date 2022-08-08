#include <Python.h>
#include "../include/PyTuple.hpp"

PyObject* PyTuple::get(int n) {

    return PyTuple_GetItem(tuple, n);

}

Py_ssize_t PyTuple::getSize() {

    return PyTuple_GET_SIZE(tuple);

}
