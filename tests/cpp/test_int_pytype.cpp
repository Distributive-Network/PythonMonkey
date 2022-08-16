#include <gtest/gtest.h>
#include <Python.h>
#include "../../include/IntType.hpp"
#include <iostream>
#include <string>

#include "include/TypeEnum.hpp"
#include "include/utilities.hpp"


class IntTypeTests : public ::testing::Test {
protected:
    PyObject* i_type;
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

    EXPECT_TRUE(instanceof<PyType>(&x));
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

TEST_F(IntTypeTests, test_cout_type_correctly) {

    IntType my_int = IntType(i_type);

    std::string expected = "10";
    testing::internal::CaptureStdout();
    std::cout << my_int;
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(expected, output);

}

TEST_F(IntTypeTests, test_factor_correctly) {
    IntType my_int = IntType(i_type);

    PyObject* expected = Py_BuildValue("[i, i, i, i]", 1, 2, 5, 10);
    PyObject* output = my_int.factor();

    EXPECT_EQ(PyList_GetItem(expected, 0), PyList_GetItem(output, 0));
    EXPECT_EQ(PyList_GetItem(expected, 1), PyList_GetItem(output, 1));
    EXPECT_EQ(PyList_GetItem(expected, 2), PyList_GetItem(output, 2));
    EXPECT_EQ(PyList_GetItem(expected, 3), PyList_GetItem(output, 3));
}

