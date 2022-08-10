#ifndef DICTTYPE_HPP
#define DICTTYPE_HPP

#include <Python.h>

#include "PyType.hpp"

class DictType: public PyType {
    protected:
        const TYPE returnType = TYPE::DICT;
        virtual void print(std::ostream& os) const override;
    
    public:
        DictType(PyObject* object);
        TYPE getReturnType() const;
};

#endif