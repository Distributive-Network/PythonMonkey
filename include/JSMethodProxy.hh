/**
 * @file JSMethodProxy.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief JSMethodProxy is a custom C-implemented python type. It acts as a proxy for JSFunctions from Spidermonkey, and behaves like a method would, treating `self` as `this`.
 * @date 2023-11-14
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSMethodProxy_
#define PythonMonkey_JSMethodProxy_

#include "include/JSFunctionProxy.hh"

#include <jsapi.h>

#include <Python.h>
/**
 * @brief The typedef for the backing store that will be used by JSMethodProxy objects. All it contains is a pointer to the JSFunction and a pointer to self
 *
 */
typedef struct {
  PyObject_HEAD
  PyObject *self;
  JS::PersistentRootedObject *jsFunc;
} JSMethodProxy;

/**
 * @brief This struct is a bundle of methods used by the JSMethodProxy type
 *
 */
struct JSMethodProxyMethodDefinitions {
public:
/**
 * @brief Deallocation method (.tp_dealloc), removes the reference to the underlying JSFunction before freeing the JSMethodProxy
 *
 * @param self - The JSMethodProxy to be free'd
 */
  static void JSMethodProxy_dealloc(JSMethodProxy *self);

  /**
   * @brief New method (.tp_new), creates a new instance of the JSMethodProxy type, exposed as the __new()__ method in python
   *
   * @param type - The type of object to be created, will always be JSMethodProxyType or a derived type
   * @param args - arguments to the __new()__ method, expected to be a JSFunctionProxy, and an object to bind self to
   * @param kwds - keyword arguments to the __new()__ method, not used
   * @return PyObject* - A new instance of JSMethodProxy
   */
  static PyObject *JSMethodProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

  /**
   * @brief Call method (.tp_call), called when the JSMethodProxy is called, properly handling `self` and `this`
   *
   * @param self - the JSMethodProxy being called
   * @param args - args to the method
   * @param kwargs - keyword args to the method
   * @return PyObject* - Result of the method call
   */
  static PyObject *JSMethodProxy_call(PyObject *self, PyObject *args, PyObject *kwargs);
};

/**
 * @brief Struct for the JSMethodProxyType, used by all JSMethodProxy objects
 */
extern PyTypeObject JSMethodProxyType;

#endif