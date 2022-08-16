#include <gtest/gtest.h>

#include "include/IntType.hpp"
#include "include/StrType.hpp"
#include "include/DictType.hpp"
#include "include/PyTypeFactory.hpp"
#include "include/utilities.hpp"

class PyTypeFactoryTests : public ::testing::Test {
protected:
    PyObject* i_type; 
    PyObject* s_type;
    PyObject* dict_type;
    // PyObject *func_type; On hold for now since I don't know how to construct these from the api yet
    virtual void SetUp() {      
        Py_Initialize();
        i_type = Py_BuildValue("i", 10);
        s_type = Py_BuildValue("s", (char*)"something");
        dict_type = Py_BuildValue("{s:i, s:i}", (char*)"a", 10, (char*)"b", 12);
        Py_XINCREF(i_type);
    }
    
    virtual void TearDown() {
        Py_XDECREF(i_type);
    }
};

TEST_F(PyTypeFactoryTests, test_makes_int_type_correctly) {
    
    PyType* obj = PyTypeFactory(i_type);

    EXPECT_TRUE(instanceof<IntType>(&*obj));

}

TEST_F(PyTypeFactoryTests, test_makes_str_type_correctly) {
    PyType* obj = PyTypeFactory(s_type); 

    EXPECT_TRUE(instanceof<StrType>(&*obj));
}

TEST_F(PyTypeFactoryTests, test_makes_dict_type_correctly) {
    PyType* obj = PyTypeFactory(dict_type);

    EXPECT_TRUE(instanceof<DictType>(&*obj));
}

