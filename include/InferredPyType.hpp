#ifndef INFERREDPYTYPES_HPP
#define INFERREDPYTYPES_HPP

#include <Python.h>
#include <string>
#include <any>

class  InferredPyType {

private:
PyObject* input;

std::string inferred_type;
char* string_identifier;
std::any inferred_value;

void inferTypes(PyObject* object);

public:

InferredPyType(PyObject* _input);

std::string getInferedType(); 

char* getStringIdentifier();

template<typename T>
T cast();

};

#endif