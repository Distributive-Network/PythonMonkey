#include "include/PyEvaluator.hh"
#include "include/PyTuple.hh"

#include <gtest/gtest.h>
#include <Python.h>

class PyEvaluatorTests : public ::testing::Test {
protected:
PyObject *i_type;
virtual void SetUp() {
  Py_Initialize();
}

virtual void TearDown() {}
};

TEST_F(PyEvaluatorTests, test_evaluates_simple_string_correctly) {
  // Static, since it doesn't really make sense to keep an instance of this spun up
  PyEvaluator p;

  std::string expected = "hello world";
  testing::internal::CaptureStdout();
  p.eval("print('hello world')");
  std::string output = testing::internal::GetCapturedStdout();
}

TEST_F(PyEvaluatorTests, test_can_evaluate_and_run_a_function) {
  PyEvaluator p;
  PyObject* arg_tuple = Py_BuildValue("(i)", 10);
  PyTuple* args = new PyTuple(arg_tuple);
  
  p.eval("def f(x)\n\treturn x * 5", args);
}
