#ifndef Bifrost_PyType_
#define Bifrost_PyType_

#include "TypeEnum.hh"

#include <Python.h>

#include <iostream>

/**
 * @brief Abstract Class that serves as a base for the different type relations in C++/Python
 */
class PyType {
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