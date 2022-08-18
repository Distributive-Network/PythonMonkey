#ifndef Bifrost_PyTypeFactory_
#define Bifrost_PyTypeFactory_

#include <Python.h>
#include "PyType.hpp"

/** @brief Function that takes an arbitrary PyObject* and returns a corresponding PyType* object

    @author Caleb Aikens
    @date August 2022

    @param object - Pointer to the PyObject who's type and value we wish to encapsulate
 */
PyType *PyTypeFactory(PyObject *object);
#endif