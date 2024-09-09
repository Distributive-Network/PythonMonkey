/**
 * @file JSStringProxy.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief JSStringProxy is a custom C-implemented python type that derives from str. It acts as a proxy for JSStrings from Spidermonkey, and behaves like a str would.
 * @date 2024-01-03
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSStringProxy_
#define PythonMonkey_JSStringProxy_

#include <jsapi.h>

#include <Python.h>

#include <unordered_set>

/**
 * @brief The typedef for the backing store that will be used by JSStringProxy objects. All it contains is a pointer to the JSString
 *
 */
typedef struct {
  PyUnicodeObject str;
  JS::PersistentRootedValue *jsString;
} JSStringProxy;

extern std::unordered_set<JSStringProxy *> jsStringProxies; // a collection of all JSStringProxy objects, used during a GCCallback to ensure they continue to point to the correct char buffer

/**
 * @brief This struct is a bundle of methods used by the JSStringProxy type
 *
 */
struct JSStringProxyMethodDefinitions {
public:
  /**
   * @brief Deallocation method (.tp_dealloc), removes the reference to the underlying JSString before freeing the JSStringProxy
   *
   * @param self - The JSStringProxy to be free'd
   */
  static void JSStringProxy_dealloc(JSStringProxy *self);

  /**
   * @brief copy protocol method for both copy and deepcopy
   *
   * @param self - The JSObjectProxy
   * @return a copy of the string
   */
  static PyObject *JSStringProxy_copy_method(JSStringProxy *self);
};

// docs for methods, copied from cpython
PyDoc_STRVAR(stringproxy_deepcopy__doc__,
  "__deepcopy__($self, memo, /)\n"
  "--\n"
  "\n");

PyDoc_STRVAR(stringproxy_copy__doc__,
  "__copy__($self, /)\n"
  "--\n"
  "\n");

/**
 * @brief Struct for the other methods
 *
 */
static PyMethodDef JSStringProxy_methods[] = {
  {"__deepcopy__", (PyCFunction)JSStringProxyMethodDefinitions::JSStringProxy_copy_method, METH_O, stringproxy_deepcopy__doc__}, // ignores any memo argument
  {"__copy__", (PyCFunction)JSStringProxyMethodDefinitions::JSStringProxy_copy_method, METH_NOARGS, stringproxy_copy__doc__},
  {NULL, NULL}                  /* sentinel */
};

/**
 * @brief Struct for the JSStringProxyType, used by all JSStringProxy objects
 */
extern PyTypeObject JSStringProxyType;

#endif