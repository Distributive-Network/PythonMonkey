#ifndef PYTYPE_HPP
#define PYTYPE_HPP

#include <Python.h>
#include <string>

/**
 * @brief Abstract Class that serves as a base for the different type relations in C++/Python
 */
class PyType {

protected:
PyObject* object;

public:
    PyType(PyObject* _object) {};
};
#endif