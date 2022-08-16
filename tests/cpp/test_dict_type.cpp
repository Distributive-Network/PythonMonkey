#include <gtest/gtest.h>
#include <Python.h>
#include <iostream>
#include <string>

#include "include/TypeEnum.hpp"
#include "include/PyType.hpp"
#include "include/DictType.hpp"
#include "include/StrType.hpp"
#include "include/IntType.hpp"


template<typename Base, typename T>
inline bool instanceof(const T *ptr) {
   return dynamic_cast<const Base*>(ptr) != nullptr;
}

class DictTypeTests : public ::testing::Test {
protected:
    PyObject* dict_type;
    PyObject* default_key;
    PyObject* default_value;
    virtual void SetUp() {      
        Py_Initialize();
        dict_type = PyDict_New();
        default_key = Py_BuildValue("s", (char*)"a");
        default_value = Py_BuildValue("i", 10);

        PyDict_SetItem(dict_type, default_key, default_value);

        Py_XINCREF(dict_type);
        Py_XINCREF(default_key);
        Py_XINCREF(default_value);
    }

    virtual void TearDown() {
        Py_XINCREF(dict_type);
        Py_XINCREF(default_key);
        Py_XINCREF(default_value);
    }
};

TEST_F(DictTypeTests, test_dict_type_instance_of_pytype) { 

    DictType dict = DictType(dict_type);

    EXPECT_TRUE(instanceof<PyType>(&dict));

}

TEST_F(DictTypeTests, test_sets_values_appropriately) {

    DictType dict = DictType(dict_type);

    StrType *key = new StrType((char*)"c");
    IntType *value = new IntType(15);

    dict.set(key, value); 

    PyObject* expected = value->getPyObject();
    PyObject* set_value = PyDict_GetItem(dict.getPyObject(), key->getPyObject());
    
    EXPECT_EQ(set_value, expected);

    delete key;
    delete value;
}

TEST_F(DictTypeTests, test_gets_existing_values_appropriately) {

    DictType dict = DictType(dict_type);

    StrType *key = new StrType((char *)"a");

    auto get_value = dict.get(key);
    PyType* expected = new IntType(default_value);

    PyObject* get_value_object = get_value.value()->getPyObject();

    EXPECT_EQ(get_value_object, default_value);
    // EXPECT_TRUE(instanceof<PyType>(get_value_object));
}

TEST_F(DictTypeTests, test_get_returns_null_when_getting_non_existent_value) {

    DictType dict = DictType(dict_type);

    StrType *key = new StrType((char*)"b");

    EXPECT_FALSE(dict.get(key).has_value());

}

TEST_F(DictTypeTests, test_print_overload_prints_basic_types_correctly) {
    PyObject* dict_with_standard_types = Py_BuildValue("{s:i, s:s, s:i}", 
                                                        (char*)"a", 10, (char*)"b", 
                                                        (char*)"c", (char*)"d", 12);

    DictType my_dict = DictType(dict_with_standard_types);
    
    std::string expected = "{\n  'a':10,\n  'b':'c',\n  'd':12\n}";
    testing::internal::CaptureStdout();
    std::cout << my_dict;
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(expected, output);
}

TEST_F(DictTypeTests, test_print_overload_prints_nested_dictionaries_correctly) {
    PyObject* dict_with_standard_types = Py_BuildValue("{s:i, s:s, s:i}", 
                                                        (char*)"a", 10, (char*)"b", 
                                                        (char*)"c", (char*)"d", 12);

    PyObject* dict_to_nest = Py_BuildValue("{s:i, s:i}", (char*)"nested_a", 13, (char*)"nested_b", 14);

    PyDict_SetItemString(dict_with_standard_types, (char*)"nested_dict", dict_to_nest);

    DictType my_dict = DictType(dict_with_standard_types);
    
    std::string expected = "{\n  'a':10,\n  'b':'c',\n  'd':12,\n  'nested_dict':{\n    'nested_a':13,\n    'nested_b':14\n  }\n}";
    testing::internal::CaptureStdout();
    std::cout << my_dict;
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(expected, output);
}