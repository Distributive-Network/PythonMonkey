/**
 * @file FuncType.hh
 * @author Caleb Aikens (caleb@distributive.network), Giovanni Tedesco (giovanni@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct representing python functions
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_FuncType_
#define PythonMonkey_FuncType_

#include <jsapi.h>

#include <Python.h>

/**
 * @brief This struct represents the 'function' type in Python
 */
struct FuncType {
public:
  static PyObject *getPyObject(JSContext *cx, JS::HandleValue fval);
};

#endif