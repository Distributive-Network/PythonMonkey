#ifndef INTPYTYPE_HPP
#define INTPYTYPE_HPP

#include <string>
#include "PyType.hpp"

/**
 * @brief This class represents the 'int' type in C. It inherits from the PyType class
 */
class IntType : public PyType {
    private:
        std::string returnType = "int";
        std::string stringIdentifier = "%d";
        int value;
    
    protected:
        virtual void print(std::ostream& os) const override;

    public:
        // using PyType::PyType;
        IntType(PyObject* _object): PyType(_object) {
            value = PyLong_AS_LONG(object);
        }

        ~IntType();
        // Virtual Methods
        virtual std::string getStringIdentifier() override;
        virtual std::string getReturnType() override;
        virtual PyObject* getPyObject() override;

        // Find a way to show that this type of thing should be virtual.
        int cast();

        // Find a way to have an interface for this.
        static IntType from_c_type(int x);
};

#endif