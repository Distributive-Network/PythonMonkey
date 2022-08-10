#include <include/DictType.hpp>
#include <Python.h>

#include <include/TypeEnum.hpp>

DictType::DictType(PyObject* object): PyType(object) {
}

void DictType::print(std::ostream& os) const {
    os << "something for now";
}