/**
 * @file ListType.hh
 * @author Caleb Aikens (caleb@distributive.network), Giovanni Tedesco (giovanni@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing python lists
 * @date 2022-08-18
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_ListType_
#define PythonMonkey_ListType_

#include <jsapi.h>

#include <Python.h>


/**
 * @brief This struct represents a list in python
 *
 * @author Giovanni
 */
struct ListType {
public:
  static PyObject *getPyObject(JSContext *cx, JS::HandleObject arrayObj);
};
#endif