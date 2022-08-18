#ifndef Bifrost_FuncType_
#define Bifrost_FuncType_

#include <string>
#include "PyType.hh"
#include "TypeEnum.hh"

/**
 * @brief This class represents the 'function' type in Python. It inherits from the PyType class
 */
class FuncType : public PyType {
protected:
const TYPE returnType = TYPE::FUNC;
virtual void print(std::ostream &os) const override;

public:
FuncType(PyObject *object);
const char *getValue() const;
};

#endif