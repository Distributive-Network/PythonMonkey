#include <gtest/gtest.h>
#include <Python.h>
#include <string>
#include "../../include/InferredPyType.hpp"

class InferredTypeTests : public ::testing::Test {
protected:
    virtual void SetUp() {      
        Py_Initialize();
    }

    virtual void TearDown() {

    }
};

TEST_F(InferredTypeTests, create_generic_type) {
    PyObject* i_type = Py_BuildValue("i", 10);
    Py_INCREF(i_type);

    InferredPyType obj(i_type);
    
    Py_XDECREF(i_type);
}

TEST_F(InferredTypeTests, infer_object_c_type_integers) {
    PyObject* i_type = Py_BuildValue("i", 10);
    Py_INCREF(i_type);

    InferredPyType obj(i_type);

    std::string expected = "int";

    EXPECT_EQ(obj.getInferedType(), expected);
    
    Py_XDECREF(i_type);
}

TEST_F(InferredTypeTests, infer_object_c_type_strings) {
    PyObject* s_type = Py_BuildValue("s", (char *)"something");
    Py_INCREF(s_type);

    InferredPyType obj(s_type);

    EXPECT_EQ(obj.getInferedType(), "str");
    
    Py_XDECREF(s_type);
}

TEST_F(InferredTypeTests, get_printf_identifier_integer) {
    PyObject* i_type = Py_BuildValue("i", 10);
    Py_INCREF(i_type);

    InferredPyType obj(i_type);

    // EXPECT_STREQ(obj.getStringIdentifier(), "%d");
    
    Py_XDECREF(i_type);
}

TEST_F(InferredTypeTests, get_printf_identifier_string) {
    PyObject* s_type = Py_BuildValue("s", (char *)"something");
    Py_INCREF(s_type);

    InferredPyType obj(s_type);

    // EXPECT_STREQ(obj.getStringIdentifier(), "%s");

    Py_XDECREF(s_type);
}

// TEST_F(TypeTests, cast_pyobject_to_type_int) {

//     PyObject* i_type = Py_BuildValue("i", 10);

//     InferredPyType obj(i_type);

//     int value = obj.cast<int>();

//     int expected = 10;

//     EXPECT_EQ(value, expected);
// }

// TEST_F(TypeTests, cast_pyobject_to_type_string) {
//     PyObject* s_type = Py_BuildValue("s", (char*)"something");

//     InferredPyType obj(s_type);

//     char * value = obj.cast<char*>();

//     char* expected = (char*)"something";

//     EXPECT_EQ(value, expected);
// }
