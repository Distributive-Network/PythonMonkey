/**
 * @file NullType.hh
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing JS null in a python object
 * @date 2023-02-22
 *
 * @copyright Copyright (c) 2023,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_NullType_
#define PythonMonkey_NullType_

#include <Python.h>

/**
 * @brief This struct represents the JS null type in Python using a singleton object on the pythonmonkey module
 */
struct NullType {
public:
  static PyObject *getPyObject();
};

#endif