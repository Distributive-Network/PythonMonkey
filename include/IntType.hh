/**
 * @file IntType.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network) & Tom Tang (xmader@distributive.network)
 * @brief Struct for representing python ints
 * @version 0.2
 * @date 2023-03-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef PythonMonkey_IntType_
#define PythonMonkey_IntType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <jsapi.h>

#include <Python.h>

/**
 * @brief This struct represents the 'int' type (arbitrary-precision) in Python. It inherits from the PyType struct
 */
struct IntType : public PyType {
public:
  IntType(PyObject *object);
  IntType(long n);

  /**
   * @brief Construct a new IntType object from a JS::BigInt.
   *
   * @param cx - javascript context pointer
   * @param bigint - JS::BigInt pointer
   */
  IntType(JSContext *cx, JS::BigInt *bigint);

  const TYPE returnType = TYPE::INT;

  /**
   * @brief Convert the IntType object to a JS::BigInt
   *
   * @param cx - javascript context pointer
   */
  JS::BigInt *toJsBigInt(JSContext *cx);
};

#endif