/**
 * @file FuncType.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Struct representing python functions
 * @version 0.1
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef PythonMonkey_FuncType_
#define PythonMonkey_FuncType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <Python.h>

#include <iostream>

/**
 * @brief This struct represents the 'function' type in Python. It inherits from the PyType struct
 */
struct FuncType : public PyType {
public:
  FuncType(PyObject *object);
  TYPE getReturnType() override;
  const char *getValue() const;
};

#endif