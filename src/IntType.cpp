#include "../include/IntType.hpp"

std::string IntType::getReturnType() {
    return returnType;
}

std::string IntType::getStringIdentifier() {
    return stringIdentifier;
}

PyObject* IntType::getPyObject() {
    return object;
}

int IntType::cast() {
    return (int)PyLong_AS_LONG(object);
}
