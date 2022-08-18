#ifndef Bifrost_FuncType_
#define Bifrost_FuncType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <Python.h>

#include <iostream>

/**
 * @brief This class represents the 'function' type in Python. It inherits from the PyType class
 */
class FuncType : public PyType {
public:
FuncType(PyObject *object);
const TYPE returnType = TYPE::FUNC;
const char *getValue() const;

protected:
virtual void print(std::ostream &os) const override;
};

#endif