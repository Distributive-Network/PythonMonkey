/**
 * @file NullType.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing JS null in a python object
 * @date 2023-02-22
 *
 * @copyright Copyright (c) 2023,2024 Distributive Corp.
 *
 */

#include "include/NullType.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"

PyObject *NullType::getPyObject() {
  PyObject *pmNull = getPythonMonkeyNull();
  Py_INCREF(pmNull);
  return pmNull;
}