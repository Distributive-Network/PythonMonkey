/**
 * @file test_tuple_type.cc
 * @author Giovanni Tedesco
 * @brief Tests for the tuple type class
 * @version 0.1
 * @date 2022-08-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "include/TupleType.hh"

#include "include/PyType.hh"

#include <gtest/gtest.h>
#include <Python.h>

#include <iostream>
#include <string>

class TupleTypeTests : public ::testing::Test {
protected:
PyObject *tuple_type;
virtual void SetUp() {
  Py_Initialize();
  tuple_type = PyTuple_New(1);

  Py_XINCREF(tuple_type);

}

virtual void TearDown() {
  Py_XDECREF(tuple_type);

}
};

TEST_F(TupleTypeTests, test_tuple_type_instance_of_pytype) {

  TupleType tuple = TupleType(tuple_type);

  EXPECT_TRUE(dynamic_cast<const PyType *>(&tuple) != nullptr);

}

TEST_F(TupleTypeTests, test_gets_existing_values_appropriately) {
  PyObject *test_tuple = Py_BuildValue("(i)", 10);

  TupleType tuple = TupleType(test_tuple);

  PyType *retrieved = tuple.get(0);
  PyObject *expected = Py_BuildValue("i", 10);

  EXPECT_EQ(retrieved->getPyObject(), expected);
}

TEST_F(TupleTypeTests, test_getLength_returns_correct_length) {
  PyObject *test_tuple = Py_BuildValue("(i,s,i)", 10, (char *)"hello", 12);

  TupleType my_tuple = TupleType(test_tuple);

  EXPECT_EQ(my_tuple.len(), 3);
}