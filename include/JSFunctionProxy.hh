/**
 * @file JSFunctionProxy.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief JSFunctionProxy is a custom C-implemented python type that derives from PyCFunctionObject. It acts as a proxy for JSFunctions from Spidermonkey, and behaves like a function would.
 * @version 0.1
 * @date 2023-07-05
 *
 * Copyright (c) 2023 Distributive Corp.
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
  PyCFunctionObject func;
  JS::PersistentRootedObject *jsFunction;
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
   * @brief Initialization method (.tp_init), initializes a newly created instance of JSFunctionProxy. exposed as the __init()__ method in python
   *
   * @param self - The JSFunctionProxy to be initialized
   * @param args - arguments to the __init()__ method, expected to be a dict
   * @param kwds - keyword arguments to the __init()__ method, not used
   * @return int - -1 on exception, return any other value otherwise
   */
  static int JSFunctionProxy_init(JSFunctionProxy *self, PyObject *args, PyObject *kwds);

  /**
   * @brief Length method (.mp_length), returns the number of key-value pairs in the JSFunction, used by the python len() method
   *
   * @param self - The JSFunctionProxy
   * @return Py_ssize_t The length of the JSFunctionProxy
   */
  static Py_ssize_t JSFunctionProxy_length(JSFunctionProxy *self);

  /**
   * @brief Getter method (.mp_subscript), returns a value from the JSFunctionProxy given a key, used by several built-in python methods as well as the [] operator
   *
   * @param self - The JSFunctionProxy
   * @param key - The key for the value in the JSFunctionProxy
   * @return PyObject* NULL on exception, the corresponding value otherwise
   */
  static PyObject *JSFunctionProxy_get(JSFunctionProxy *self, PyObject *key);

  /**
   * @brief Assign method (.mp_ass_subscript), assigns a key-value pair if value is non-NULL, or deletes a key-value pair if value is NULL
   *
   * @param self - The JSFunctionProxy
   * @param key - The key to be set or deleted
   * @param value If NULL, the key-value pair is deleted, if not NULL then a key-value pair is assigned
   * @return int -1 on exception, any other value otherwise
   */
  static int JSFunctionProxy_assign(JSFunctionProxy *self, PyObject *key, PyObject *value);

  // @TODO (Caleb Aikens) remove?
  /**
   * @brief Comparison method (.tp_richcompare), returns appropriate boolean given a comparison operator and other pyobject
   *
   * @param self - The JSFunctionProxy
   * @param other - Any other PyObject
   * @param op - Which boolean operator is being performed (Py_EQ for equality, Py_NE for inequality, all other operators are not implemented)
   * @return PyObject* - True or false depending on result of comparison
   */
  static PyObject *JSFunctionProxy_richcompare(JSFunctionProxy *self, PyObject *other, int op);

  /**
   * @brief Compute a string representation of the JSFunctionProxy
   *
   * @param self - The JSFunctionProxy
   * @return PyObject* - the string representation (a PyUnicodeObject) on success, NULL on failure
   */
  static PyObject *JSFunctionProxy_repr(JSFunctionProxy *self);

  /**
   * @brief Function to call the JSFunctionProxy
   *
   * @param self - The JSFunctionProxy being called
   * @param args - tuple of ordered arguments
   * @param kwargs - dictionary of keyword arguments, can be NULL
   * @return PyObject* - the result of the function call
   */
  static PyObject *JSFunctionProxy_call(JSFunctionProxy *self, PyObject *args, PyObject *kwargs);

  /**
   * @brief Traversal function for python garbage collection
   *
   * @param self - Pointer to this JSfunctionProxy
   * @param visit - unused
   * @param arg - unused
   * @return int - 0 on success
   */
  static int JSFunctionProxy_traverse(JSFunctionProxy *self, visitproc visit, void *arg);
};

/**
 * @brief Struct for the JSFunctionProxyType, used by all JSFunctionProxy objects
 */
extern PyTypeObject JSFunctionProxyType;

#endif