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

#include <jsapi.h>

#include <Python.h>

/**
 * @brief This struct represents the 'function' type in Python. It inherits from the PyType struct
 */
struct FuncType : public PyType {
public:
  FuncType(PyObject *object);
  FuncType(JSContext *cx, JS::HandleValue fval);
  const TYPE returnType = TYPE::FUNC;
  const char *getValue() const;
};

#endif