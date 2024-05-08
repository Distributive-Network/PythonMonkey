/**
 * @file JSObjectIterProxy.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSObjectIterProxy is a custom C-implemented python type that derives from PyDictIterKey
 * @date 2024-01-17
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSObjectIterProxy_
#define PythonMonkey_JSObjectIterProxy_


#include <jsapi.h>

#include <Python.h>

#define KIND_KEYS 0
#define KIND_VALUES 1
#define KIND_ITEMS 2


/**
 * @brief The typedef for the backing store that will be used by JSObjectIterProxy objects.
 *
 */

typedef struct {
  PyObject_HEAD
  JS::PersistentRootedIdVector *props;
  int it_index;
  bool reversed;
  int kind;
  PyDictObject *di_dict;   /* Set to NULL when iterator is exhausted */
} dictiterobject;


typedef struct {
  dictiterobject it;
} JSObjectIterProxy;

/**
 * @brief This struct is a bundle of methods used by the JSArrayProxy type
 *
 */
struct JSObjectIterProxyMethodDefinitions {
public:
  /**
   * @brief Deallocation method (.tp_dealloc), removes the reference to the underlying JSObject before freeing the JSArrayProxy
   *
   * @param self - The JSObjectIterProxy to be free'd
   */
  static void JSObjectIterProxy_dealloc(JSObjectIterProxy *self);

  /**
   * @brief .tp_traverse method
   *
   * @param self - The JSObjectIterProxy
   * @param visit - The function to be applied on each element of the list
   * @param arg - The argument to the visit function
   * @return 0 on success
   */
  static int JSObjectIterProxy_traverse(JSObjectIterProxy *self, visitproc visit, void *arg);

  /**
   * @brief .tp_clear method
   *
   * @param self - The JSObjectIterProxy
   * @return 0 on success
   */
  static int JSObjectIterProxy_clear(JSObjectIterProxy *self);

  /**
   * @brief .tp_iter method
   *
   * @param self - The JSObjectIterProxy
   * @return PyObject* - an interator over the iterator
   */
  static PyObject *JSObjectIterProxy_iter(JSObjectIterProxy *self);

  /**
   * @brief .tp_next method
   *
   * @param self - The JSObjectIterProxy
   * @return PyObject* - next object in iteration
   */
  static PyObject *JSObjectIterProxy_nextkey(JSObjectIterProxy *self);

  /**
   * @brief length method
   *
   * @param self - The JSObjectIterProxy
   * @return PyObject* - number of objects left to iterate over in iteration
   */
  static PyObject *JSObjectIterProxy_len(JSObjectIterProxy *self);
};


PyDoc_STRVAR(dict_length_hint_doc, "Private method returning an estimate of len(list(it)).");

/**
 * @brief Struct for the other methods
 *
 */
static PyMethodDef JSObjectIterProxy_methods[] = {
  {"__length_hint__", (PyCFunction)JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_len, METH_NOARGS, dict_length_hint_doc},
  {NULL, NULL}                       /* sentinel */
};

/**
 * @brief Struct for the JSArrayProxyType, used by all JSArrayProxy objects
 */
extern PyTypeObject JSObjectIterProxyType;

#endif