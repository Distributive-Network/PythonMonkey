/**
 * @file FloatType.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing python floats
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#include "include/FloatType.hh"

PyObject *FloatType::getPyObject(double n) {
  PyObject *doubleVal = Py_BuildValue("d", n);
  Py_INCREF(doubleVal);
  return doubleVal;
}