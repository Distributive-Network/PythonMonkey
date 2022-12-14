#ifndef PythonMonkey_Explore_
#define PythonMonkey_Explore_

#include "include/IntType.hh"
#include "include/ListType.hh"

#include <Python.h>

/** @brief Function that takes an arbitrary number of arguments from python and outputs their C/C++ values.

    @author Giovanni Tedesco & Caleb Aikens
    @date July 2022

    @param self - Pointer to the python environment
    @param args - The PyTuple of arguments that are passed into the function
 */
static PyObject *output(PyObject *self, PyObject *args);
static PyObject *factor(PyObject *self, PyObject *args);

/**
 * @brief
 *
 * @param self - Pointer to the python environment
 * @param args - The PyTuple of arguments that are passed into the function
 * @return PyObject*
 */
static PyObject *run(PyObject *self, PyObject *args);

/**
 * @brief Function that factors an integer in python
 *
 * @param self - Pointer to python environment
 * @param args  - The PyTuple of arugments that are passed into the function
 * @return PyObject*
 */
static PyObject *pfactor(PyObject *self, PyObject *args);

ListType *factor_int(IntType *x);

#endif