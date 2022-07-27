#ifndef INTPYTYPE_HPP
#define INTPYTYPE_HPP

#include <string>
#include "PyType.hpp"

class IntType : public PyType {
    public:
        using PyType::PyType;

        virtual std::string getStringIdentifier() override;
        virtual std::string getReturnType() override;

        static IntType cast();
};

#endif