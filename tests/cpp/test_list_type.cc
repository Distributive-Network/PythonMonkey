#include <gtest/gtest.h>
#include <Python.h>
#include <iostream>
#include <string>

#include "include/TypeEnum.hh"
#include "include/PyType.hh"
#include "include/ListType.hh"
#include "include/StrType.hh"
#include "include/IntType.hh"

#include "include/utilities.hh"

class ListTypeTests : public ::testing::Test {
protected:
PyObject *list_type;
virtual void SetUp() {
  Py_Initialize();
  list_type = PyList_New(0);

  Py_XINCREF(list_type);

}

virtual void TearDown() {
  Py_XDECREF(list_type);

}
};

TEST_F(ListTypeTests, test_list_type_instance_of_pytype) {

  ListType list = ListType(list_type);

  EXPECT_TRUE(instanceof<PyType>(&list));

}

TEST_F(ListTypeTests, test_sets_values_appropriately) {

  ListType list = ListType(list_type);

}

TEST_F(ListTypeTests, test_gets_existing_values_appropriately) {

  ListType list = ListType(list_type);
}

TEST_F(ListTypeTests, test_get_returns_null_when_getting_non_existent_value) {}