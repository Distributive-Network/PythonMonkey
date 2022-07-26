#include <gtest/gtest.h>
#include <Python.h>
#include "../../include/PyTuple.hpp"

TEST(TupleTests, create_tuple) {
    Py_Initialize();
    // Figure out this test later ...
    PyObject* input = Py_BuildValue("(ii)", 1, 2);

    PyTuple my_tuple(input);

    Py_XDECREF(input);

    // This doesn't have an assertion but the fact that it doesn't fail is a sign
    // that the object is being constructed correctly. For now this is fine
}

TEST(TupleTests, get_one_item) {

    Py_Initialize();

    PyObject* input = Py_BuildValue("(ii)", 1, 2);
    Py_XINCREF(input);

    PyTuple my_py_tuple(input);

    Py_ssize_t size = PyTuple_GET_SIZE(input);

    PyObject* expected = PyTuple_GetItem(input, 0);

    EXPECT_EQ(my_py_tuple.get(0), expected);


    Py_XDECREF(input);
}

// Temporary until I can figure out how the exceptions work
// TEST(TupleTests, out_of_bounds_throws_exception) {

//     Py_Initialize();

//     PyObject* input = Py_BuildValue("(ii)", 1, 2);

//     PyTuple my_py_tuple(input);

//     // Temporary until I can figure out finding the specific throw
//     EXPECT_ANY_THROW(my_py_tuple.get(-1));
//     EXPECT_ANY_THROW(my_py_tuple.get(3));


//     Py_XDECREF(input);
// }

TEST(TupleTests, test_get_size) { 

    Py_Initialize();

    PyObject* input = Py_BuildValue("(ii)", 1, 2);
    Py_XINCREF(input);

    PyTuple my_py_tuple(input);

    Py_ssize_t expected_size = 2;

    EXPECT_EQ(my_py_tuple.getSize(), expected_size);

    Py_XDECREF(input);
}

