/**
 * @file FloatType.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for representing python floats
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022 Distributive Corp.
 *
 */

#include "include/FloatType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <Python.h>

FloatType::FloatType(PyObject *object) : PyType(object) {}

FloatType::FloatType(long n) : PyType(Py_BuildValue("d", (double)n)) {}

FloatType::FloatType(double n) : PyType(Py_BuildValue("d", n)) {}

double FloatType::getValue() const {
  return PyFloat_AS_DOUBLE(pyObject);
}