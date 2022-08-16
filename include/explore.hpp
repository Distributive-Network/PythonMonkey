#ifndef EXPLORE_H
#define EXPLORE_H

#include <Python.h>

/** @brief Function that takes an arbitrary number of arguments from python and outputs their C/C++ values.

    @author Giovanni Tedesco & Caleb Aikens
    @date July 2022
    
    @param self - Pointer to the python environment
    @param args - The PyTuple of arguments that are passed into the function
    */
static PyObject* output(PyObject* self, PyObject *args);
static PyObject* factor(PyObject* self, PyObject *factor);

#endif