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

    public:
        using PyType::PyType;
        // Virtual Methods
        virtual std::string getStringIdentifier() override;
        virtual std::string getReturnType() override;

        // Find a way to show that this type of thing should be virtual.
        int cast();
};

#endif