/**
 * @file JSArrayIterProxy.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSArrayIterProxy is a custom C-implemented python type that derives from PyListIter
 * @version 0.1
 * @date 2024-01-15
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSArrayIterProxy_
#define PythonMonkey_JSArrayIterProxy_


#include <jsapi.h>

#include <Python.h>


// redeclare hidden type
typedef struct {
  PyObject_HEAD
  int it_index;
  bool reversed;
  PyListObject *it_seq;   /* Set to NULL when iterator is exhausted */
} PyListIterObject;

/**
 * @brief The typedef for the backing store that will be used by JSArrayIterProxy objects.
 *
 */
typedef struct {
  PyListIterObject it;
} JSArrayIterProxy;

/**
 * @brief This struct is a bundle of methods used by the JSArrayProxy type
 *
 */
struct JSArrayIterProxyMethodDefinitions {
public:
  /**
   * @brief Deallocation method (.tp_dealloc), removes the reference to the underlying JSObject before freeing the JSArrayProxy
   *
   * @param self - The JSArrayIterProxy to be free'd
   */
  static void JSArrayIterProxy_dealloc(JSArrayIterProxy *self);

  /**
   * @brief .tp_traverse method
   *
   * @param self - The JSArrayIterProxy
   * @param visit - The function to be applied on each element of the list
   * @param arg - The argument to the visit function
   * @return 0 on success
   */
  static int JSArrayIterProxy_traverse(JSArrayIterProxy *self, visitproc visit, void *arg);

  /**
   * @brief .tp_clear method
   *
   * @param self - The JSArrayIterProxy
   * @return 0 on success
   */
  static int JSArrayIterProxy_clear(JSArrayIterProxy *self);

  /**
   * @brief .tp_iter method
   *
   * @param self - The JSArrayIterProxy
   * @return PyObject* - an interator over the iterator
   */
  static PyObject *JSArrayIterProxy_iter(JSArrayIterProxy *self);

  /**
   * @brief .tp_next method
   *
   * @param self - The JSArrayIterProxy
   * @return PyObject* - next object in iteration
   */
  static PyObject *JSArrayIterProxy_next(JSArrayIterProxy *self);

  /**
   * @brief length method
   *
   * @param self - The JSArrayIterProxy
   * @return PyObject* - number of objects left  to iterate over in iteration
   */
  static PyObject *JSArrayIterProxy_len(JSArrayIterProxy *self);
};


PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

/**
 * @brief Struct for the other methods
 *
 */
static PyMethodDef JSArrayIterProxy_methods[] = {
  {"__length_hint__", (PyCFunction)JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_len, METH_NOARGS, length_hint_doc},
  {NULL, NULL}                       /* sentinel */
};

/**
 * @brief Struct for the JSArrayProxyType, used by all JSArrayProxy objects
 */
extern PyTypeObject JSArrayIterProxyType;

#endif