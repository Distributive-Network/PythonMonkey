/**
 * @file BoolType.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing python bools
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#include "include/BoolType.hh"


PyObject *BoolType::getPyObject(long n) {
  return PyBool_FromLong(n);
}