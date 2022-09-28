/**
 * @file IntType.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Struct for representing python ints
 * @version 0.1
 * @date 2022-07-27
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef Bifrost_IntType_
#define Bifrost_IntType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <Python.h>

#include <iostream>

/**
 * @brief This struct represents the 'int' type in Python, which is represented as a 'long' in C++. It inherits from the PyType struct
 */
struct IntType : public PyType {
public:
  IntType(PyObject *object);
  IntType(long n);
  const TYPE returnType = TYPE::INT;
  long getValue() const;

protected:
  virtual void print(std::ostream &os) const override;
};

#endif