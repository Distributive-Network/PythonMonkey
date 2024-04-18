/**
 * @file FloatType.hh
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing python floats
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_FloatType_
#define PythonMonkey_FloatType_

#include <Python.h>

/**
 * @brief This struct represents the 'float' type in Python, which is represented as a 'double' in C++
 */
struct FloatType {
public:
  static PyObject *getPyObject(double n);
};

#endif