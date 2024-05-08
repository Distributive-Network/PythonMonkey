/**
 * @file JSObjectKeysProxy.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSObjectKeysProxy is a custom C-implemented python type that derives from dict keys
 * @date 2024-01-16
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSObjectKeysProxy_
#define PythonMonkey_JSObjectKeysProxy_

#include <jsapi.h>

#include <Python.h>


/**
 * @brief The typedef for the backing store that will be used by JSObjectKeysProxy objects
 *
 */
typedef struct {
  _PyDictViewObject dv;
} JSObjectKeysProxy;

/**
 * @brief This struct is a bundle of methods used by the JSObjectKeysProxy type
 *
 */
struct JSObjectKeysProxyMethodDefinitions {
public:
  /**
   * @brief Deallocation method (.tp_dealloc), removes the reference to the underlying JSObject before freeing the JSObjectKeysProxy
   *
   * @param self - The JSObjectKeysProxy to be free'd
   */
  static void JSObjectKeysProxy_dealloc(JSObjectKeysProxy *self);

  /**
   * @brief .tp_traverse method
   *
   * @param self - The JSObjectKeysProxy
   * @param visit - The function to be applied on each element of the list
   * @param arg - The argument to the visit function
   * @return 0 on success
   */
  static int JSObjectKeysProxy_traverse(JSObjectKeysProxy *self, visitproc visit, void *arg);

  /**
   * @brief .tp_clear method
   *
   * @param self - The JSObjectKeysProxy
   * @return 0 on success
   */
  static int JSObjectKeysProxy_clear(JSObjectKeysProxy *self);

  /**
   * @brief Length method (.sq_length), returns the number of key-value pairs in the JSObject, used by the python len() method
   *
   * @param self - The JSObjectProxy
   * @return Py_ssize_t The length of the JSObjectProxy
   */
  static Py_ssize_t JSObjectKeysProxy_length(JSObjectKeysProxy *self);

  /**
   * @brief Test method (.sq_contains), returns whether a key exists, used by the in operator
   *
   * @param self - The JSObjectKeysProxy
   * @param key - The key for the value in the JSObjectKeysProxy
   * @return int 1 if `key` is in dict, 0 if not, and -1 on error
   */
  static int JSObjectKeysProxy_contains(JSObjectKeysProxy *self, PyObject *key);

  /**
   * @brief Comparison method (.tp_richcompare), returns appropriate boolean given a comparison operator and other pyobject
   *
   * @param self - The JSObjectKeysProxy
   * @param other - Any other PyObject
   * @param op - Which boolean operator is being performed (Py_EQ for equality, Py_NE for inequality, all other operators are not implemented)
   * @return PyObject* - True or false depending on result of comparison
   */
  static PyObject *JSObjectKeysProxy_richcompare(JSObjectKeysProxy *self, PyObject *other, int op);

  /**
   * @brief Return an iterator object to make JSObjectKeysProxy iterable, emitting (key, value) tuples
   *
   * @param self - The JSObjectKeysProxy
   * @return PyObject* - iterator object
   */
  static PyObject *JSObjectKeysProxy_iter(JSObjectKeysProxy *self);

  /**
   * @brief Compute a string representation of the JSObjectKeysProxy
   *
   * @param self - The JSObjectKeysProxy
   * @return the string representation (a PyUnicodeObject) on success, NULL on failure
   */
  static PyObject *JSObjectKeysProxy_repr(JSObjectKeysProxy *self);

  /**
   * @brief Set intersect operation
   *
   * @param self - The JSObjectKeysProxy
   * @param other - The other PyObject to be and'd, expected to be dict or JSObjectKeysProxy
   * @return PyObject* The resulting new dict
   */
  static PyObject *JSObjectKeysProxy_intersect(JSObjectKeysProxy *self, PyObject *other);

  /**
   * @brief Set disjoint method
   *
   * @param self - The JSObjectKeysProxy
   * @param other - The other PyObject to be and'd, expected to be dict or JSObjectKeysProxy
   * @return PyObject* The resulting new dict
   */
  static PyObject *JSObjectKeysProxy_isDisjoint(JSObjectKeysProxy *self, PyObject *other);

  /**
   * @brief reverse iterator method
   *
   * @param self - The JSObjectKeysProxy
   * @return PyObject* The resulting new dict
   */
  static PyObject *JSObjectKeysProxy_iter_reverse(JSObjectKeysProxy *self);

  /**
   * @brief mapping method
   *
   * @param self - The JSObjectKeysProxy
   * @param Py_UNUSED
   * @return PyObject* The resulting new dict
   */
  static PyObject *JSObjectKeysProxy_mapping(PyObject *self, void *Py_UNUSED(ignored));
};

/**
 * @brief Struct for the methods that define the Sequence protocol
 *
 */
static PySequenceMethods JSObjectKeysProxy_sequence_methods = {
  .sq_length = (lenfunc)JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_length,
  .sq_contains = (objobjproc)JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_contains
};

static PyNumberMethods JSObjectKeysProxy_number_methods = {
  // .nb_subtract = default is fine
  .nb_and =  (binaryfunc)JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_intersect,
  // .nb_xor = default is fine
  // .nb_or = default is fine
};

PyDoc_STRVAR(isdisjoint_doc,
  "Return True if the view and the given iterable have a null intersection.");

PyDoc_STRVAR(reversed_keys_doc,
  "Return a reverse iterator over the dict keys.");

/**
 * @brief Struct for the other methods
 *
 */
static PyMethodDef JSObjectKeysProxy_methods[] = {
  {"isdisjoint", (PyCFunction)JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_isDisjoint, METH_O, isdisjoint_doc},
  {"__reversed__", (PyCFunction)JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_iter_reverse, METH_NOARGS, reversed_keys_doc},
  {NULL, NULL}                  /* sentinel */
};

static PyGetSetDef JSObjectKeysProxy_getset[] = {
  {"mapping", JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_mapping, (setter)NULL, "dictionary that this view refers to", NULL},
  {0}
};

/**
 * @brief Struct for the JSObjectKeysProxyType, used by all JSObjectKeysProxy objects
 */
extern PyTypeObject JSObjectKeysProxyType;

#endif