/**
 * @file PyEvaluator.cc
 * @author your name (you@domain.com)
 * @brief Implementation of the methods of the PyEvaluator class
 * @version 0.1
 * @date 2022-08-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "include/DictType.hh"
#include "include/PyEvaluator.hh"
#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"
#include "include/TupleType.hh"


#include <Python.h>

#include <iostream>
#include <string>

PyEvaluator::PyEvaluator() {

  this->py_module = PyModule_New("Bifrost2");
  PyModule_AddStringConstant(this->py_module, "__file__", "");
  this->py_global = new DictType(PyDict_New());
  this->py_local = new DictType(PyModule_GetDict(this->py_module));

}

PyEvaluator::~PyEvaluator() {}

void PyEvaluator::eval(const std::string &input) {

  PyRun_SimpleString(input.c_str());

}

PyType *PyEvaluator::eval(const std::string &input, const std::string &func_name, TupleType *args) {

  PyObject *py_create_func = PyRun_String(input.c_str(), Py_file_input, this->py_global->getPyObject(), this->py_local->getPyObject());

// pValue would be null if the Python syntax is wrong, for example
  if (py_create_func == NULL) {
    if (PyErr_Occurred()) {
      std::cout << "Something wrong happened";
    }
    return nullptr;
  }

  Py_DECREF(py_create_func);

  PyObject *py_func = PyObject_GetAttrString(this->py_module, func_name.c_str());

  // Double check we have actually found it and it is callable
  if (!py_func || !PyCallable_Check(py_func)) {
    if (PyErr_Occurred()) {
      std::cout << "function is not callable";
    }
    std::cerr << "Cannot find function: " << func_name << std::endl;
    return nullptr;
  }

  PyObject *function_return = PyObject_CallObject(py_func, args->getPyObject());

  return pyTypeFactory(function_return);

}