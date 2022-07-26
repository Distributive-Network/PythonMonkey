#include <iostream>
#include <Python.h>

static PyObject* say_hello(PyObject* self, PyObject *args) {

    std::cout << "hello world" << std::endl;

    Py_RETURN_NONE;
}

static PyMethodDef ExploreMethods[] = {
    {"say_hello", say_hello, METH_VARARGS, "Says hello!"},
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
