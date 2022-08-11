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