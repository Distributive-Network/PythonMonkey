/**
 * @file TupleType.cc
 * @author Giovanni Tedesco (giovanni@distributive.network)
 * @brief Struct for representing python tuples
 * @date 2022-08-19
 *
 * @copyright Copyright (c) 2022 Distributive Corp.
 *
 */

#include "include/TupleType.hh"

#include <Python.h>

#include <string>

TupleType::TupleType(PyObject *object) : PyType(object) {}