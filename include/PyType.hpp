#ifndef PYTYPE_HPP
#define PYTYPE_HPP

#include <Python.h>
#include <string>
#include <iostream>

#include "TypeEnum.hpp"

/**
 * @brief Abstract Class that serves as a base for the different type relations in C++/Python
 */
class PyType {

protected:
PyObject *pyObject;
const TYPE returnType = TYPE::DEFAULT;
virtual void print(std::ostream &os) const = 0;

public:
PyType(PyObject *object);
~PyType();

PyObject *getPyObject();
TYPE getReturnType();

friend std::ostream &operator <<(std::ostream &str, const PyType &data)
{
  data.print(str);
  return str;
}
};
#endif