#include "include/DictType.hh"
#include "include/PyEvaluator.hh"
#include "include/TupleType.hh"

#include <Python.h>

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

void PyEvaluator::eval(const std::string &input, const std::string &func_name, TupleType *args) {

  PyObject *py_create_func = PyRun_String(input.c_str(), Py_file_input, this->py_global->getPyObject(), this->py_local->getPyObject());

  PyObject *py_func = PyObject_GetAttrString(this->py_module, func_name.c_str());

  PyObject *function_return = PyObject_CallObject(py_func, args->getPyObject());
}