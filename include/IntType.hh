/**
 * @file IntType.hh
 * @author Caleb Aikens (caleb@distributive.network), Giovanni Tedesco (giovanni@distributive.network), Tom Tang (xmader@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing python ints
 * @date 2023-03-16
 *
 * @copyright Copyright (c) 2023,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_IntType_
#define PythonMonkey_IntType_

#include <jsapi.h>

#include <Python.h>

/**
 * @brief This struct represents the 'int' type (arbitrary-precision) in Python
 */
struct IntType {
public:
  /**
   * @brief Construct a new PyObject from a JS::BigInt.
   *
   * @param cx - javascript context pointer
   * @param bigint - JS::BigInt pointer
   */
  static PyObject *getPyObject(JSContext *cx, JS::BigInt *bigint);

  /**
   * @brief Convert the IntType object to a JS::BigInt
   *
   * @param cx - javascript context pointer
   */
  static JS::BigInt *toJsBigInt(JSContext *cx, PyObject *pyObject);
};

#endif