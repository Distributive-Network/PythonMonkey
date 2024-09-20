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

#endif // #ifndef PythonMonkey_py_version_shim_
