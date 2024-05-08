/**
 * @file NoneType.hh
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing None
 * @date 2023-02-22
 *
 * @copyright Copyright (c) 2023,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_NoneType_
#define PythonMonkey_NoneType_

#include <Python.h>

/**
 * @brief This struct represents the 'None' type in Python
 */
struct NoneType {
public:
  static PyObject *getPyObject();

};

#endif