/**
 * @file JSFunctionProxy.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief JSFunctionProxy is a custom C-implemented python type. It acts as a proxy for JSFunctions from Spidermonkey, and behaves like a function would.
 * @date 2023-09-28
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSFunctionProxy_
#define PythonMonkey_JSFunctionProxy_

#include <jsapi.h>

#include <Python.h>
/**
 * @brief The typedef for the backing store that will be used by JSFunctionProxy objects. All it contains is a pointer to the JSFunction
 *
 */
typedef struct {
  PyObject_HEAD
  JS::PersistentRootedObject *jsFunc;
} JSFunctionProxy;

/**
 * @brief This struct is a bundle of methods used by the JSFunctionProxy type
 *
 */
struct JSFunctionProxyMethodDefinitions {
public:
/**
 * @brief Deallocation method (.tp_dealloc), removes the reference to the underlying JSFunction before freeing the JSFunctionProxy
 *
 * @param self - The JSFunctionProxy to be free'd
 */
  static void JSFunctionProxy_dealloc(JSFunctionProxy *self);

  /**
   * @brief New method (.tp_new), creates a new instance of the JSFunctionProxy type, exposed as the __new()__ method in python
   *
   * @param type - The type of object to be created, will always be JSFunctionProxyType or a derived type
   * @param args - arguments to the __new()__ method, not used
   * @param kwds - keyword arguments to the __new()__ method, not used
   * @return PyObject* - A new instance of JSFunctionProxy
   */
  static PyObject *JSFunctionProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

  /**
   * @brief Call method (.tp_call), called when the JSFunctionProxy is called
   *
   * @param self - this callable, might be a free function or a method
   * @param args - args to the function
   * @param kwargs - keyword args to the function
   * @return PyObject* - Result of the function call
   */
  static PyObject *JSFunctionProxy_call(PyObject *self, PyObject *args, PyObject *kwargs);
};

/**
 * @brief Struct for the JSFunctionProxyType, used by all JSFunctionProxy objects
 */
extern PyTypeObject JSFunctionProxyType;

#endif