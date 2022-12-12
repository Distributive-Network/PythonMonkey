/**
 * @file pyTypeFactory.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Function for wrapping arbitrary PyObjects into the appropriate PyType class
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef PythonMonkey_PyTypeFactory_
#define PythonMonkey_PyTypeFactory_

#include "PyType.hh"

#include <Python.h>

/** @brief Function that takes an arbitrary PyObject* and returns a corresponding PyType* object

    @author Caleb Aikens
    @date August 2022

    @param object - Pointer to the PyObject who's type and value we wish to encapsulate
 */
PyType *pyTypeFactory(PyObject *object);

#endif