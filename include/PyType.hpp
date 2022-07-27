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
    PyType(PyObject* _object): object(_object) {};

    virtual std::string getReturnType() = 0;
    virtual std::string getStringIdentifier() = 0;
    virtual PyObject* getPyObject() = 0;

    // Is there some way to override constructors... I don't know.
};
#endif