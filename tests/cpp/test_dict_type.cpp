#include <gtest/gtest.h>
#include <Python.h>
#include "../../include/IntType.hpp"
#include <iostream>
#include <string>
#include "include/TypeEnum.hpp"

template<typename Base, typename T>
inline bool instanceof(const T *ptr) {
   return dynamic_cast<const Base*>(ptr) != nullptr;
}

class DictTypeTests : public ::testing::Test {
protected:
    PyObject* dict_type;
    PyObject* key;
    PyObject* value;
    virtual void SetUp() {      
        Py_Initialize();
        dict_type = PyDict_New();
        key = Py_BuildValue("s", (char*)"a");
        value = Py_BuildValue("i", 10);

        PyDict_SetItem(dict_type, key, value);

        Py_XINCREF(dict_type);
        Py_XINCREF(key);
        Py_XINCREF(value);
    }

    virtual void TearDown() {
        Py_XINCREF(dict_type);
        Py_XINCREF(key);
        Py_XINCREF(value);
    }
};
