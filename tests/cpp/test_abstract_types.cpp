#include <gtest/gtest.h>
#include <Python.h>
#include "../../include/PyType.hpp"

class AbstractPyTypeTests : public ::testing::Test {
protected:
    virtual void SetUp() {      
        Py_Initialize();
    }

    virtual void TearDown() {

    }
};

TEST_F(AbstractPyTypeTests, can_create_PyType) {
    PyObject* i_type = Py_BuildValue("i", 10);   
    Py_XINCREF(i_type);

    PyType abs_type(i_type);

    Py_XDECREF(i_type);
}
