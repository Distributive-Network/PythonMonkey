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
    PyObject* object;
    virtual void print(std::ostream& os) const = 0;

public:
    PyType(PyObject* _object): object(_object) {};

    virtual std::string getReturnType() = 0;
    virtual std::string getStringIdentifier() = 0;
    virtual PyObject* getPyObject() = 0;

    friend std::ostream& operator<<(std::ostream& str, const PyType& data)
    {
        data.print(str);
        return str;
    }
};
#endif