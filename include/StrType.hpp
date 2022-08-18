#ifndef STRPYTYPE_HPP
#define STRPYTYPE_HPP

#include <string>
#include "PyType.hpp"
#include "TypeEnum.hpp"

/**
 * @brief This class represents the 'string' type in Python, which is represented as a 'char*' in C++. It inherits from the PyType class
 */
class StrType : public PyType {
protected:
const TYPE returnType = TYPE::STRING;
virtual void print(std::ostream &os) const override;

public:
StrType(PyObject *object);
StrType(char *string);
const char *getValue() const;
TYPE getReturnType() const;
};

#endif