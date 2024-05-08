/**
 * @file DictType.hh
 * @author Caleb Aikens (caleb@distributive.network), Giovanni Tedesco (giovanni@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct representing python dictionaries
 * @date 2022-08-10
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_DictType_
#define PythonMonkey_DictType_

#include <jsapi.h>

#include <Python.h>

/**
 * @brief This struct represents a dictionary in python.
 *
 * @author Giovanni
 */
struct DictType {
public:
  /**
   * @brief Construct a new DictType object from a JSObject.
   *
   * @param cx - pointer to the JSContext
   * @param jsObject - pointer to the JSObject to be coerced
   *
   * @returns PyObject* pointer to the resulting PyObject
   */
  static PyObject *getPyObject(JSContext *cx, JS::Handle<JS::Value> jsObject);
};

#endif