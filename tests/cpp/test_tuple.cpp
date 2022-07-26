#include <gtest/gtest.h>
#include <Python.h>
#include "../include/PyTuple.hpp"

TEST(TupleTests, create_tuple) {

    // Figure out this test later ...
    PyObject* input = PyTuple_Pack(2, "a", "b");

    PyTuple my_tuple(input);

    Py_XDECREF(input);

}

TEST(TupleTests, get_one_item) {

    PyObject* input = PyTuple_Pack(2, "a", "b");

    PyTuple my_py_tuple(input);

    Py_ssize_t size = PyTuple_GET_SIZE(input);

    PyObject* expected = PyTuple_GetItem(input, 0);

    EXPECT_EQ(my_py_tuple.get(0), expected);


    Py_XDECREF(input);
}

TEST(TupleTests, out_of_bounds_throws_exception) {

    PyObject* input = PyTuple_Pack(2, "a", "b");

    PyTuple my_py_tuple(input);

    EXPECT_EQ(my_py_tuple.get(-1), NULL);
    EXPECT_EQ(my_py_tuple.get(3), NULL);


    Py_XDECREF(input);
}

TEST(TupleTests, test_get_size) { 
    
    PyObject* input = PyTuple_Pack(2, "a", "b");

    PyTuple my_py_tuple(input);

    Py_ssize_t expected_size = 2;

    EXPECT_EQ(my_py_tuple.getSize(), expected_size);

    Py_XDECREF(input);
}

