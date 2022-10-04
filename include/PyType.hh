/**
 * @file PyType.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network)
 * @brief Struct representing python types
 * @version 0.1
 * @date 2022-07-27
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PythonMonkey_PyType_
#define PythonMonkey_PyType_

#include "TypeEnum.hh"

#include <Python.h>

#include <iostream>

/**
 * @brief Abstract struct that serves as a base for the different type relations in C++/Python
 */
struct PyType {
public:
  PyType(PyObject *object);
  friend std::ostream &operator <<(std::ostream &str, const PyType &data) {
    data.print(str);
    return str;
  }
  const TYPE returnType = TYPE::DEFAULT;
  PyObject *getPyObject();
  ~PyType();

protected:
  virtual void print(std::ostream &os) const = 0;
  PyObject *pyObject;
};
#endif