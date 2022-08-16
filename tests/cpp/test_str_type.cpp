#include <gtest/gtest.h>
#include <Python.h>
#include <iostream>
#include <string>

#include "include/TypeEnum.hpp"
#include "include/StrType.hpp"
#include "include/utilities.hpp"

class StrTypeTest : public ::testing::Test {
protected:
    PyObject* s_type;
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

    EXPECT_TRUE(instanceof<PyType>(&str));
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

TEST_F(StrTypeTest, test_cout_type_correctly) {

    StrType my_str = StrType(s_type);

    std::string expected = "'something'";
    testing::internal::CaptureStdout();
    std::cout << my_str;
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(expected, output);

}
