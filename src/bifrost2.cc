#include "include/bifrost2.hh"

#include <jsapi.h>
#include <js/Initialization.h>

#include <Python.h>

static JSContext *cx;

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

PyMODINIT_FUNC PyInit_bifrost2(void)
{
  if (!JS_Init())
    return NULL;

  cx = JS_NewContext(JS::DefaultHeapMaxBytes);
  if (!cx)
    return NULL;

  if (!JS::InitSelfHostedCode(cx))
    return NULL;  

  return PyModule_Create(&bifrost2);
}