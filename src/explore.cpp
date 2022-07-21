#include <Python.h>
#include <iostream>
#include <string>
#include <stdio.h>

static PyObject* output(PyObject *self, PyObject *args) {
    int size = PyTuple_Size(args);

    for(int i = 0; i < size; i++) {
        PyObject* item = PyTuple_GET_ITEM(args, i);

        if(PyUnicode_Check(item)) {
            const char *print_value = PyUnicode_AsUTF8(item);
            printf("%s", print_value);
        } else if(PyLong_Check(item)) {
            long print_value = PyLong_AS_LONG(item);
            printf("%ld", print_value);
        } else {
            printf("You are attempting to output a type that has not been implemented in this project! Aborting.");
            Py_RETURN_NONE;
        }
    }
    Py_RETURN_NONE;
}

static PyMethodDef ExploreMethods[] = {
    {"output", output, METH_VARARGS, "Multivariate function outputs"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef explore =
{
    PyModuleDef_HEAD_INIT,
    "explore",     /* name of module */
    "",          /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    ExploreMethods
};

PyMODINIT_FUNC PyInit_explore(void)
{
    return PyModule_Create(&explore);
}