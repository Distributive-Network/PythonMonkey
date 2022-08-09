#ifndef INTPYTYPE_HPP
#define INTPYTYPE_HPP

#include <string>
#include "PyType.hpp"

/**
 * @brief This class represents the 'int' type in Python, which is represented as a 'long' in C++. It inherits from the PyType class
 */
class IntType : public PyType {    
    protected:
        const std::string returnType = "int";
        virtual void print(std::ostream& os) const override;

    public:
        IntType(PyObject* object);
        IntType(long n);
        long getValue() const;
        std::string getReturnType();

};

#endif