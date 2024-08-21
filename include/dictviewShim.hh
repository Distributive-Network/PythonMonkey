/**
 * @file dictviewShim.hh
 * @author Tom Tang (xmader@distributive.network)
 * @brief Since Python 3.13, `_PyDictView` moved from Python's public API to the **internal** header file `internal/pycore_dict.h`.
 *        This file behaves as a shim to bring back its definitions.
 *        The code here is copied from https://github.com/python/cpython/blob/v3.13.0rc1/Include/internal/pycore_dict.h#L64-L72.
 * @date 2024-08-21
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_dictview_shim_
#define PythonMonkey_dictview_shim_

#include <Python.h>

/* _PyDictView */

typedef struct {
  PyObject_HEAD
  PyDictObject *dv_dict;
} _PyDictViewObject;

extern PyObject *_PyDictView_New(PyObject *, PyTypeObject *);
extern PyObject *_PyDictView_Intersect(PyObject *self, PyObject *other);

#endif
