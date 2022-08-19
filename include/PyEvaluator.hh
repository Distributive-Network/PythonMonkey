#ifndef Bifrost_PyEvaluator_
#define Bifrost_PyEvaluator_

#include "include/TupleType.hh"
#include "include/DictType.hh"

#include <Python.h>

#include <string>

/**
 * @brief A class that is used for evaluating python programs inside of C++ using the embedding api
 * provided by python.
 * 
 * @author Giovanni Tedesco
 */
class PyEvaluator {
private:
PyObject *py_module; // The module that our custom functions will be contained in.
DictType *py_local; // python dictionry that stores local context (i think?)
DictType *py_global; // python dictionary that stores global context

public:
PyEvaluator();
~PyEvaluator();

/**
 * @brief Simple python program evaluation method. This is not meant to handle input, it will simply run
 * the python program as described in the input string
 *
 * @param input A string defining the python program
 */
void eval(const std::string &input);

/**
 * @brief Evaluation method for python functions. This will store the python function defined in the input string,
 * then evaluate it using the arguments in args
 *
 * @param input The python function input
 * @param args The arguments to evaluate the python function at.
 */
PyType *eval(const std::string &input, const std::string &func_name, TupleType *args);

};

#endif