#ifndef PYTYPE_HPP
#define PYTYPE_HPP

#include <Python.h>
#include <string>
#include <iostream>

/**
 * @brief Abstract Class that serves as a base for the different type relations in C++/Python
 */
class PyType {

protected:
PyObject *pyObject;
const std::string returnType;
virtual void print(std::ostream &os) const = 0;

public:
PyType(PyObject *object);
~PyType();

PyObject *getPyObject();
std::string getReturnType();

friend std::ostream &operator <<(std::ostream &str, const PyType &data)
{
  data.print(str);
  return str;
}
};
#endif