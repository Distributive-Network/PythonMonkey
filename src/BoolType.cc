/**
 * @file BoolType.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for representing python bools
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "include/BoolType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <Python.h>

BoolType::BoolType(PyObject *object) : PyType(object) {}

BoolType::BoolType(long n) : PyType(PyBool_FromLong(n)) {}

long BoolType::getValue() const {
  return PyLong_AS_LONG(pyObject);
}