/**
 * @file BoolType.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for representing python bools
 * @version 0.1
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PythonMonkey_BoolType_
#define PythonMonkey_BoolType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <Python.h>

/**
 * @brief This struct represents the 'bool' type in Python, which is represented as a 'long' in C++. It inherits from the PyType struct
 */
struct BoolType : public PyType {
public:
  BoolType(PyObject *object);
  BoolType(long n);
  const TYPE returnType = TYPE::BOOL;
  long getValue() const;
};

#endif