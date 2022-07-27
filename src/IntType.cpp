#include "../include/IntType.hpp"

inline IntType::IntType(PyObject* _object) : PyType(_object) {
    value = (int)PyLong_AS_LONG(object);
}

std::string IntType::getReturnType() {
    return returnType;
}

std::string IntType::getStringIdentifier() {
    return stringIdentifier;
}

IntType IntType::cast(PyObject* object) {
    return NULL;
}

int IntType::getValue() {
    return value;
}