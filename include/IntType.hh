#ifndef Bifrost_IntType_
#define Bifrost_IntType_

#include <Python.h>
#include <string>
#include "PyType.hh"
#include "TypeEnum.hh"

/**
 * @brief This class represents the 'int' type in Python, which is represented as a 'long' in C++. It inherits from the PyType class
 */
class IntType : public PyType {
protected:
const TYPE returnType = TYPE::INT;
virtual void print(std::ostream &os) const override;

public:
IntType(PyObject *object);
IntType(long n);
long getValue() const;
TYPE getReturnType() const;

};

#endif