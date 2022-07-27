#include <gtest/gtest.h>
#include <Python.h>
#include "../../include/IntType.hpp"

template<typename Base, typename T>
inline bool instanceof(const T *ptr) {
   return dynamic_cast<const Base*>(ptr) != nullptr;
}

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

    std::string expected = "int";

    EXPECT_EQ(x.getReturnType(), "int");
}

TEST_F(IntTypeTests, test_returns_correct_value) {
    IntType x = IntType(i_type);

    int expected = 10;

    EXPECT_EQ(x.getValue(), 10); 
}

TEST_F(IntTypeTests, test_returns_correct_string_identifier) {
    IntType x = IntType(i_type);

    std::string expected = "%d";

    EXPECT_EQ(x.getStringIdentifier(), expected); 
}
