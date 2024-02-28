/**
 * @file TupleType.cc
 * @author Giovanni Tedesco
 * @brief Implementation for the methods of the Tuple Type struct
 * @version 0.1
 * @date 2022-08-19
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "include/TupleType.hh"

#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"

#include <Python.h>

#include <string>

TupleType::TupleType(PyObject *object) : PyType(object) {}