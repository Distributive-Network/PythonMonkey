/**
 * @file NoneType.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing None
 * @date 2023-02-22
 *
 * @copyright Copyright (c) 2023,2024 Distributive Corp.
 *
 */

#include "include/NoneType.hh"

PyObject *NoneType::getPyObject() {
  Py_INCREF(Py_None);
  return Py_None;
}