#include <gtest/gtest.h>
#include <Python.h>
#include <iostream>
#include <string>

#include "include/TypeEnum.hh"
#include "include/StrType.hh"

class StrTypeTest : public ::testing::Test {
protected:
PyObject *s_type;
virtual void SetUp() {
  Py_Initialize();
  s_type = Py_BuildValue("s", (char *)"something");
  Py_XINCREF(s_type);
}


virtual void TearDown() {
  Py_XDECREF(s_type);
}
};

TEST_F(StrTypeTest, test_can_create_string_type_from_pyobject) {
  StrType str = StrType(s_type);

  EXPECT_TRUE(dynamic_cast<const PyType *>(&str) != nullptr);
}

TEST_F(StrTypeTest, test_returns_correct_return_type_for_str) {
  StrType str = StrType(s_type);

  TYPE expected = TYPE::STRING;

  EXPECT_EQ(str.getReturnType(), expected);
}

TEST_F(StrTypeTest, test_getPyObject_returns_correct_PyObject) {
  StrType x = StrType(s_type);

  EXPECT_EQ(x.getPyObject(), s_type);
}