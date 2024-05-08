/**
 * @file JSObjectItemsProxy.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSObjectItemsProxy is a custom C-implemented python type that derives from dict items
 * @date 2024-01-19
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSObjectItemsProxy_
#define PythonMonkey_JSObjectItemsProxy_

#include <jsapi.h>

#include <Python.h>


/**
 * @brief The typedef for the backing store that will be used by JSObjectItemsProxy objects
 *
 */
typedef struct {
  _PyDictViewObject dv;
} JSObjectItemsProxy;

/**
 * @brief This struct is a bundle of methods used by the JSObjectItemsProxy type
 *
 */
struct JSObjectItemsProxyMethodDefinitions {
public:
  /**
   * @brief Deallocation method (.tp_dealloc), removes the reference to the underlying JSObject before freeing the JSObjectItemsProxy
   *
   * @param self - The JSObjectItemsProxy to be free'd
   */
  static void JSObjectItemsProxy_dealloc(JSObjectItemsProxy *self);

  /**
   * @brief .tp_traverse method
   *
   * @param self - The JSObjectItemsProxy
   * @param visit - The function to be applied on each element of the list
   * @param arg - The argument to the visit function
   * @return 0 on success
   */
  static int JSObjectItemsProxy_traverse(JSObjectItemsProxy *self, visitproc visit, void *arg);

  /**
   * @brief .tp_clear method
   *
   * @param self - The JSObjectItemsProxy
   * @return 0 on success
   */
  static int JSObjectItemsProxy_clear(JSObjectItemsProxy *self);

  /**
   * @brief Length method (.sq_length), returns the number of key-value pairs in the JSObject, used by the python len() method
   *
   * @param self - The JSObjectProxy
   * @return Py_ssize_t The length of the JSObjectProxy
   */
  static Py_ssize_t JSObjectItemsProxy_length(JSObjectItemsProxy *self);

  /**
   * @brief Return an iterator object to make JSObjectItemsProxy iterable, emitting (key, value) tuples
   *
   * @param self - The JSObjectItemsProxy
   * @return PyObject* - iterator object
   */
  static PyObject *JSObjectItemsProxy_iter(JSObjectItemsProxy *self);

  /**
   * @brief Compute a string representation of the JSObjectItemsProxy
   *
   * @param self - The JSObjectItemsProxy
   * @return the string representation (a PyUnicodeObject) on success, NULL on failure
   */
  static PyObject *JSObjectItemsProxy_repr(JSObjectItemsProxy *self);

  /**
   * @brief reverse iterator method
   *
   * @param self - The JSObjectItemsProxy
   * @return PyObject* The resulting new dict
   */
  static PyObject *JSObjectItemsProxy_iter_reverse(JSObjectItemsProxy *self);

  /**
   * @brief mapping method
   *
   * @param self - The JSObjectItemsProxy
   * @param Py_UNUSED
   * @return PyObject* The resulting new dict
   */
  static PyObject *JSObjectItemsProxy_mapping(PyObject *self, void *Py_UNUSED(ignored));
};

/**
 * @brief Struct for the methods that define the Sequence protocol
 *
 */
static PySequenceMethods JSObjectItemsProxy_sequence_methods = {
  .sq_length = (lenfunc)JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_length,
  // .sq_contains = TODO tuple support
};

PyDoc_STRVAR(items_reversed_keys_doc,
  "Return a reverse iterator over the dict keys.");

/**
 * @brief Struct for the other methods
 *
 */
static PyMethodDef JSObjectItemsProxy_methods[] = {
  // {"isdisjoint"}, // TODO tuple support
  {"__reversed__", (PyCFunction)JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_iter_reverse, METH_NOARGS, items_reversed_keys_doc},
  {NULL, NULL}                  /* sentinel */
};

static PyGetSetDef JSObjectItemsProxy_getset[] = {
  {"mapping", JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_mapping, (setter)NULL, "dictionary that this view refers to", NULL},
  {0}
};

/**
 * @brief Struct for the JSObjectItemsProxyType, used by all JSObjectItemsProxy objects
 */
extern PyTypeObject JSObjectItemsProxyType;

#endif