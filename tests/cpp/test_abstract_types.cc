#include <gtest/gtest.h>
#include <Python.h>
#include "include/PyType.hh"

class AbstractPyTypeTests : public ::testing::Test {
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

// TODO: Figure out how to test the abstract class