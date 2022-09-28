/**
 * @file StrType.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Struct for representing python strings
 * @version 0.1
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef Bifrost_StrType_
#define Bifrost_StrType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <Python.h>

#include <iostream>

/**
 * @brief This struct represents the 'string' type in Python, which is represented as a 'char*' in C++. It inherits from the PyType struct
 */
struct StrType : public PyType {
public:
  StrType(PyObject *object);
  StrType(char *string);
  const TYPE returnType = TYPE::STRING;
  const char *getValue() const;

protected:
  virtual void print(std::ostream &os) const override;
};

#endif