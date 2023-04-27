#include <gtest/gtest.h>
#include <Python.h>
#include <iostream>
#include <string>

#include "include/IntType.hh"
#include "include/TypeEnum.hh"
class IntTypeTests : public ::testing::Test {
protected:
PyObject *i_type;
virtual void SetUp() {
  Py_Initialize();
  i_type = Py_BuildValue("i", 10);
  Py_XINCREF(i_type);
}


virtual void TearDown() {
  Py_XDECREF(i_type);
}
};

TEST_F(IntTypeTests, test_can_create_IntType) {
  // Test that an object can be created
  // If this test passes than the constructor works
  IntType x = IntType(i_type);

  EXPECT_TRUE(dynamic_cast<const PyType *>(&x) != nullptr);
}

TEST_F(IntTypeTests, test_returns_correct_return_type_for_int) {
  IntType x = IntType(i_type);

  TYPE expected = TYPE::INT;

  EXPECT_EQ(x.getReturnType(), expected);
}

TEST_F(IntTypeTests, test_getPyObject_returns_correct_PyObject) {
  IntType x = IntType(i_type);

  EXPECT_EQ(x.getPyObject(), i_type);
}