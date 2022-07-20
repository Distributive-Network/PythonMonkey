#include <Python.h>
#include <iostream>
#include <string>
#include <stdio.h>

static PyObject *
greet_name(PyObject *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "s", &name))
    {
        return NULL;
    }

    printf("Hello %s!\n", name);

    Py_RETURN_NONE;
}

static PyObject*
output(PyObject *self, PyObject *args) {
    int size = PyTuple_Size(args);

    for(int i = 0; i < size; i++) {
        PyObject* item = PyTuple_GET_ITEM(args, i);
        std::string type = Py_TYPE(item)->tp_name;

        if(type == "str") {
            const char *print_value = PyUnicode_AsUTF8(item);
            printf("%s", print_value);
        } else if(type == "int") {
            long print_value = PyLong_AS_LONG(item);
            printf("%ld", print_value);
        } else {
            printf("you are attempting to output a type that has not been implemented in this project");
            Py_RETURN_NONE;
        }
    }

    Py_RETURN_NONE;

}

static PyMethodDef GreetMethods[] = {
    {"greet", greet_name, METH_VARARGS, "Greet an entity."},
    {"try_args", output, METH_VARARGS, "Trying out arguments"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef greet =
{
    PyModuleDef_HEAD_INIT,
    "greet",     /* name of module */
    "",          /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    GreetMethods
};

PyMODINIT_FUNC PyInit_hello(void)
{
    return PyModule_Create(&greet);
}