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
  list_type = PyList_New(1);

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

  PyObject *expected = Py_BuildValue("i", 10);

  IntType *insert = new IntType(expected);

  list.set(0, insert);

  PyObject *retrieved = PyList_GetItem(list.getPyObject(), 0);

  EXPECT_EQ(retrieved, expected);

}

TEST_F(ListTypeTests, test_gets_existing_values_appropriately) {

  ListType list = ListType(list_type);

  PyObject *expected = Py_BuildValue("i", 10);

  PyList_SetItem(list.getPyObject(), 0, expected);

  PyType *retrieved = list.get(0);

  EXPECT_EQ(retrieved->getPyObject(), expected);
}

TEST_F(ListTypeTests, test_append_works_properly) {

  PyObject *new_list = PyList_New(0);

  ListType list = ListType(new_list);

  PyObject *expected = Py_BuildValue("i", 10);
  Py_XINCREF(expected);

  IntType *ins = new IntType(expected);

  list.append(ins);

  PyObject *retrieved = PyList_GetItem(list.getPyObject(), 0);

  EXPECT_EQ(retrieved, expected);
  Py_XDECREF(expected);
}
