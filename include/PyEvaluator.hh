/**
 * @file PyEvaluator.hh
 * @author Giovanni Tedesco
 * @brief Class definition and method prototypes for a python code evaluator.
 * @version 0.1
 * @date 2022-08-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef Bifrost_PyEvaluator_
#define Bifrost_PyEvaluator_

#include "include/DictType.hh"
#include "include/TupleType.hh"
#include "include/PyType.hh"

#include <Python.h>

#include <string>

/**
 * @brief A class that is used for evaluating python programs inside of C++ using the embedding api
 * provided by python.
 *
 * @author Giovanni Tedesco
 */
class PyEvaluator {
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

private:
PyObject *py_module; /**< The module that our custom functions will be contained in. */
DictType *py_local; /**< python dictionary that stores local context (i think?) */
DictType *py_global; /**< python dictionary that stores global context */


};

#endif