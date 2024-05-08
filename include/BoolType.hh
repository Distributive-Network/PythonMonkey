/**
 * @file BoolType.hh
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing python bools
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_BoolType_
#define PythonMonkey_BoolType_

#include <Python.h>

/**
 * @brief This struct represents the 'bool' type in Python, which is represented as a 'long' in C++
 */
struct BoolType {
public:
  static PyObject *getPyObject(long n);
};

#endif