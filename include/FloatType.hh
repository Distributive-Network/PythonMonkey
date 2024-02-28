/**
 * @file FloatType.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for representing python floats
 * @version 0.1
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PythonMonkey_FloatType_
#define PythonMonkey_FloatType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <Python.h>

/**
 * @brief This struct represents the 'float' type in Python, which is represented as a 'double' in C++. It inherits from the PyType struct
 */
struct FloatType : public PyType {
public:
  FloatType(PyObject *object);
  FloatType(long n);
  FloatType(double n);
  const TYPE returnType = TYPE::FLOAT;
  double getValue() const;
};

#endif