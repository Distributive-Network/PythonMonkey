#include "include/bifrost2.hh"


#include <Python.h>

static PyMethodDef Bifrost2Methods[] = {
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef bifrost2 =
{
  PyModuleDef_HEAD_INIT,
  "bifrost2",     /* name of module */
  "",          /* module documentation, may be NULL */
  -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
  Bifrost2Methods
};

PyMODINIT_FUNC PyInit_explore(void)
{

  return PyModule_Create(&bifrost2);
}