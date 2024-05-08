/**
 * @file JSObjectValuesProxy.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSObjectValuesProxy is a custom C-implemented python type that derives from dict values
 * @date 2024-01-17
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSObjectValuesProxy_
#define PythonMonkey_JSObjectValuesProxy_

#include <jsapi.h>

#include <Python.h>


/**
 * @brief The typedef for the backing store that will be used by JSObjectValuesProxy objects
 *
 */
typedef struct {
  _PyDictViewObject dv;
} JSObjectValuesProxy;

/**
 * @brief This struct is a bundle of methods used by the JSObjectValuesProxy type
 *
 */
struct JSObjectValuesProxyMethodDefinitions {
public:
  /**
   * @brief Deallocation method (.tp_dealloc), removes the reference to the underlying JSObject before freeing the JSObjectValuesProxy
   *
   * @param self - The JSObjectValuesProxy to be free'd
   */
  static void JSObjectValuesProxy_dealloc(JSObjectValuesProxy *self);

  /**
   * @brief .tp_traverse method
   *
   * @param self - The JSObjectValuesProxy
   * @param visit - The function to be applied on each element of the list
   * @param arg - The argument to the visit function
   * @return 0 on success
   */
  static int JSObjectValuesProxy_traverse(JSObjectValuesProxy *self, visitproc visit, void *arg);

  /**
   * @brief .tp_clear method
   *
   * @param self - The JSObjectValuesProxy
   * @return 0 on success
   */
  static int JSObjectValuesProxy_clear(JSObjectValuesProxy *self);

  /**
   * @brief Length method (.sq_length), returns the number of key-value pairs in the JSObject, used by the python len() method
   *
   * @param self - The JSObjectProxy
   * @return Py_ssize_t The length of the JSObjectProxy
   */
  static Py_ssize_t JSObjectValuesProxy_length(JSObjectValuesProxy *self);

  /**
   * @brief Test method (.sq_contains), returns whether a key exists, used by the in operator
   *
   * @param self - The JSObjectValuesProxy
   * @param key - The key for the value in the JSObjectValuesProxy
   * @return int 1 if `key` is in dict, 0 if not, and -1 on error
   */
  static int JSObjectValuesProxy_contains(JSObjectValuesProxy *self, PyObject *key);

  /**
   * @brief Return an iterator object to make JSObjectValuesProxy iterable, emitting (key, value) tuples
   *
   * @param self - The JSObjectValuesProxy
   * @return PyObject* - iterator object
   */
  static PyObject *JSObjectValuesProxy_iter(JSObjectValuesProxy *self);

  /**
   * @brief Compute a string representation of the JSObjectValuesProxy
   *
   * @param self - The JSObjectValuesProxy
   * @return the string representation (a PyUnicodeObject) on success, NULL on failure
   */
  static PyObject *JSObjectValuesProxy_repr(JSObjectValuesProxy *self);

  /**
   * @brief reverse iterator method
   *
   * @param self - The JSObjectValuesProxy
   * @return PyObject* The resulting new dict
   */
  static PyObject *JSObjectValuesProxy_iter_reverse(JSObjectValuesProxy *self);

  /**
   * @brief mapping method
   *
   * @param self - The JSObjectValuesProxy
   * @param Py_UNUSED
   * @return PyObject* The resulting new dict
   */
  static PyObject *JSObjectValuesProxy_mapping(PyObject *self, void *Py_UNUSED(ignored));
};

/**
 * @brief Struct for the methods that define the Sequence protocol
 *
 */
static PySequenceMethods JSObjectValuesProxy_sequence_methods = {
  .sq_length = (lenfunc)JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_length,
  .sq_contains = (objobjproc)JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_contains
};

PyDoc_STRVAR(reversed_values_doc,
  "Return a reverse iterator over the dict values.");

/**
 * @brief Struct for the other methods
 *
 */
static PyMethodDef JSObjectValuesProxy_methods[] = {
  {"__reversed__", (PyCFunction)JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_iter_reverse, METH_NOARGS, reversed_values_doc},
  {NULL, NULL}                  /* sentinel */
};

static PyGetSetDef JSObjectValuesProxy_getset[] = {
  {"mapping", JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_mapping, (setter)NULL, "dictionary that this view refers to", NULL},
  {0}
};

/**
 * @brief Struct for the JSObjectValuesProxyType, used by all JSObjectValuesProxy objects
 */
extern PyTypeObject JSObjectValuesProxyType;

#endif