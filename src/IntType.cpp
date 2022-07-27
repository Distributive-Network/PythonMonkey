#include "../include/IntType.hpp"

IntType::~IntType() {
    Py_XDECREF(object); // Help if object is null for whatever reason or does not need to be decremented.
}

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

IntType IntType::from_c_type(int value) {
    PyObject* new_py_object = Py_BuildValue("i", value);
    Py_XINCREF(new_py_object);

    return IntType(new_py_object);
}