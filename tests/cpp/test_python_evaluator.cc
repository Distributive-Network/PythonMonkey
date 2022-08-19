/**
 * @file test_python_evaluator.cc
 * @author Giovanni Tedesco
 * @brief Tests for the Python Evaluator Class
 * @version 0.1
 * @date 2022-08-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "include/PyEvaluator.hh"
#include "include/TupleType.hh"

#include <gtest/gtest.h>
#include <Python.h>

#include <string.h>
#include <iostream>

class PyEvaluatorTests : public ::testing::Test {
protected:
PyObject *i_type;
virtual void SetUp() {
  Py_Initialize();
}

virtual void TearDown() {}
};

// TODO: Figure out how to test that it outputs to stdout correctly.
// TEST_F(PyEvaluatorTests, test_evaluates_simple_string_correctly) {
//   // Static, since it doesn't really make sense to keep an instance of this spun up
//   PyEvaluator p;

//   std::string expected = "hello world";
//   testing::internal::CaptureStdout();
//   // p.eval("print('hello world')\n");
//   PyRun_SimpleString("print('hello world')\n");
//   std::string output = testing::internal::GetCapturedStdout();

//   EXPECT_EQ(expected, output);
// }

TEST_F(PyEvaluatorTests, test_can_evaluate_and_run_a_function) {
  PyEvaluator p;
  PyObject *arg_tuple = Py_BuildValue("(i)", 10);
  TupleType *args = new TupleType(arg_tuple);

  std::string expected = "50";
  testing::internal::CaptureStdout();
  std::cout << *p.eval("def f(x):\n\treturn x * 5\n", "f", args);
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(expected, output);

}

TEST_F(PyEvaluatorTests, test_evaluates_pfactor_correctly) {
  PyEvaluator p;
  PyObject *arg_tuple = Py_BuildValue("(i)", 10);
  TupleType *args = new TupleType(arg_tuple);

  std::string expected = "[\n  1,\n  2,\n  5,\n  10\n]";
  testing::internal::CaptureStdout();
  std::cout << *p.eval("import math\ndef f(n):\n\treturn [x for x in range(1, n + 1) if n % x == 0]\n", "f", args);
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(expected, output);

}
