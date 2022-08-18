#ifndef FUNCPYTYPE_HPP
#define FUNCPYTYPE_HPP

#include <string>
#include "PyType.hpp"

/**
 * @brief This class represents the 'function' type in Python. It inherits from the PyType class
 */
class FuncType : public PyType {
protected:
const std::string returnType = "func";
virtual void print(std::ostream &os) const override;

public:
FuncType(PyObject *object);
const char *getValue() const;
};

#endif