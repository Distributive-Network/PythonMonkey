/**
 * @file pyshim.hh
 * @author Tom Tang (xmader@distributive.network)
 * @brief Python's C APIs are constantly changing in different versions of CPython.
 *        PythonMonkey has a wide variety of CPython versions' support. (Currently Python 3.8-3.13)
 *        This file helps our Python API calls work with different Python versions in the same code base.
 * @date 2024-09-20
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_py_version_shim_
#define PythonMonkey_py_version_shim_

#include <Python.h>

/**
 * @brief `_Py_IsFinalizing` becomes a stable API in Python 3.13,
 *          and renames to `Py_IsFinalizing`
 */
#if PY_VERSION_HEX >= 0x030d0000 // Python version is greater than 3.13
  #define Py_IsFinalizing Py_IsFinalizing
#else
  #define Py_IsFinalizing _Py_IsFinalizing
#endif

/**
 * @brief `_PyDictViewObject` type definition moved from Python's public API
 *          to the **internal** header file `internal/pycore_dict.h` in Python 3.13.
 *
 * @see https://github.com/python/cpython/blob/v3.13.0rc1/Include/internal/pycore_dict.h#L64-L72
 */
#if PY_VERSION_HEX >= 0x030d0000 // Python version is greater than 3.13
typedef struct {
  PyObject_HEAD
  PyDictObject *dv_dict;
} _PyDictViewObject;
#endif

/**
 * @brief Shim for `_PyArg_CheckPositional`.
 *        Since Python 3.13, `_PyArg_CheckPositional` function became an internal API.
 * @see Modified from https://github.com/python/cpython/blob/v3.13.0rc1/Python/getargs.c#L2738-L2780
 */
#if PY_VERSION_HEX >= 0x030d0000 // Python version is greater than 3.13
inline int _PyArg_CheckPositional(const char *name, Py_ssize_t nargs, Py_ssize_t min, Py_ssize_t max) {
  if (!name) { // _PyArg_CheckPositional may also be when unpacking a tuple
    name = "unpacked tuple"; // https://github.com/python/cpython/blob/v3.13.0rc1/Python/getargs.c#L2746
  }

  if (nargs < min) {
    PyErr_Format(
      PyExc_TypeError,
      "%.200s expected %s%zd argument%s, got %zd",
      name, (min == max ? "" : "at least "), min, min == 1 ? "" : "s", nargs);
    return 0;
  }

  if (nargs == 0) {
    return 1;
  }

  if (nargs > max) {
    PyErr_Format(
      PyExc_TypeError,
      "%.200s expected %s%zd argument%s, got %zd",
      name, (min == max ? "" : "at most "), max, max == 1 ? "" : "s", nargs);
    return 0;
  }

  return 1;
}
#endif

/**
 * @brief Shim for `_PyDictView_New`.
 *        Since Python 3.13, `_PyDictView_New` function became an internal API.
 * @see Modified from https://github.com/python/cpython/blob/v3.13.0rc1/Objects/dictobject.c#L5806-L5827
 */
inline PyObject *PyDictView_New(PyObject *dict, PyTypeObject *type) {
#if PY_VERSION_HEX < 0x030d0000 // Python version is lower than 3.13
  return _PyDictView_New(dict, type);
#else
  _PyDictViewObject *dv;
  dv = PyObject_GC_New(_PyDictViewObject, type);
  if (dv == NULL)
    return NULL;
  Py_INCREF(dict);
  dv->dv_dict = (PyDictObject *)dict;
  PyObject_GC_Track(dv);
  return (PyObject *)dv;
#endif
}

/**
 * @brief Shim for `_PyErr_SetKeyError`.
 *        Since Python 3.13, `_PyErr_SetKeyError` function became an internal API.
 */
inline void PyErr_SetKeyError(PyObject *key) {
  // Use the provided API when possible, as `PyErr_SetObject`'s behaviour is more complex than originally thought
  // see also: https://github.com/python/cpython/issues/101578
#if PY_VERSION_HEX < 0x030d0000 // Python version is lower than 3.13
  return _PyErr_SetKeyError(key);
#else
  return PyErr_SetObject(PyExc_KeyError, key);
#endif
}

/**
 * @brief Shim for `Py_SET_SIZE`.
 *        `Py_SET_SIZE` is not available in Python < 3.9
 */
#ifndef Py_SET_SIZE
static inline void _Py_SET_SIZE(PyVarObject *ob, Py_ssize_t size) {
  ob->ob_size = size;
}
#define Py_SET_SIZE(ob, size) _Py_SET_SIZE((PyVarObject *)(ob), size)
#endif

/**
 * @brief Shim for `PyObject_CallOneArg`.
 *        `PyObject_CallOneArg` is not available in Python < 3.9
 */
#if PY_VERSION_HEX < 0x03090000 // Python version is less than 3.9
inline PyObject *PyObject_CallOneArg(PyObject *func, PyObject *arg) {
  return PyObject_CallFunction(func, "O", arg);
}
#endif

/**
 * @brief Shim for `_PyLong_AsByteArray`.
 *        Python 3.13.0a4 added a new public API `PyLong_AsNativeBytes()` to replace the private `_PyLong_AsByteArray()`.
 *        But this change also modified the function signature of `_PyLong_AsByteArray()`.
 * @see https://github.com/python/cpython/issues/111140
 */
inline int PyLong_AsByteArray(PyLongObject *v, unsigned char *bytes, size_t n, bool little_endian, bool is_signed) {
#if PY_VERSION_HEX >= 0x030d0000 // Python version is 3.13 or higher
  return _PyLong_AsByteArray(v, bytes, n, little_endian, is_signed, /*with_exceptions*/ false);
#else
  return _PyLong_AsByteArray(v, bytes, n, little_endian, is_signed);
#endif
}

#endif // #ifndef PythonMonkey_py_version_shim_
