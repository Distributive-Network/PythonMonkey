#include <Python.h>
#include <string>
#include "../include/InferredPyType.hpp"

void InferredPyType::inferTypes(PyObject* input) {
    if(PyUnicode_Check(input)) {
        const char *input_value = PyUnicode_AsUTF8(input);
        inferred_value = input_value;
        string_identifier = (char*)"%s";
        inferred_type = "str";
    } else if(PyLong_Check(input)) {
        long input_value = PyLong_AS_LONG(input);
        inferred_value = (int)input_value;
        string_identifier = (char*)"%d";
        inferred_type = "int";
    } else {
        return;
    }
}

InferredPyType::InferredPyType(PyObject* _input) {
    input = _input;

    inferTypes(input);
}

std::string InferredPyType::getInferedType() {
    return inferred_type;
}

char* InferredPyType::getStringIdentifier() {
    return string_identifier;
}

template <typename T>
T InferredPyType::cast() {
    // Work on this tomorrow...
}