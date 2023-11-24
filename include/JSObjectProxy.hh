/**
 * @file JSObjectProxy.hh
 * @author Caleb Aikens (caleb@distributive.network) & Tom Tang (xmader@distributive.network)
 * @brief JSObjectProxy is a custom C-implemented python type that derives from dict. It acts as a proxy for JSObjects from Spidermonkey, and behaves like a dict would.
 * @version 0.1
 * @date 2023-06-26
 *
 * Copyright (c) 2023 Distributive Corp.
 *
 */

#include <jsapi.h>

#include <Python.h>

#include <unordered_map>

/**
 * @brief The typedef for the backing store that will be used by JSObjectProxy objects. All it contains is a pointer to the JSObject
 *
 */
typedef struct {
  PyDictObject dict;
  JS::RootedObject jsObject;
} JSObjectProxy;

/**
 * @brief This struct is a bundle of methods used by the JSObjectProxy type
 *
 */
struct JSObjectProxyMethodDefinitions {
public:
  /**
   * @brief Deallocation method (.tp_dealloc), removes the reference to the underlying JSObject before freeing the JSObjectProxy
   *
   * @param self - The JSObjectProxy to be free'd
   */
  static void JSObjectProxy_dealloc(JSObjectProxy *self);

  /**
   * @brief New method (.tp_new), creates a new instance of the JSObjectProxy type, exposed as the __new()__ method in python
   *
   * @param type - The type of object to be created, will always be JSObjectProxyType or a derived type
   * @param args - arguments to the __new()__ method, not used
   * @param kwds - keyword arguments to the __new()__ method, not used
   * @return PyObject* - A new instance of JSObjectProxy
   */
  static PyObject *JSObjectProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

  /**
   * @brief Initialization method (.tp_init), initializes a newly created instance of JSObjectProxy. exposed as the __init()__ method in python
   *
   * @param self - The JSObjectProxy to be initialized
   * @param args - arguments to the __init()__ method, expected to be a dict
   * @param kwds - keyword arguments to the __init()__ method, not used
   * @return int - -1 on exception, return any other value otherwise
   */
  static int JSObjectProxy_init(JSObjectProxy *self, PyObject *args, PyObject *kwds);

  /**
   * @brief Length method (.mp_length), returns the number of key-value pairs in the JSObject, used by the python len() method
   *
   * @param self - The JSObjectProxy
   * @return Py_ssize_t The length of the JSObjectProxy
   */
  static Py_ssize_t JSObjectProxy_length(JSObjectProxy *self);

  /**
   * @brief Getter method (.mp_subscript), returns a value from the JSObjectProxy given a key, used by several built-in python methods as well as the [] operator
   *
   * @param self - The JSObjectProxy
   * @param key - The key for the value in the JSObjectProxy
   * @return PyObject* NULL on exception, the corresponding value otherwise
   */
  static PyObject *JSObjectProxy_get(JSObjectProxy *self, PyObject *key);

  /**
   * @brief Assign method (.mp_ass_subscript), assigns a key-value pair if value is non-NULL, or deletes a key-value pair if value is NULL
   *
   * @param self - The JSObjectProxy
   * @param key - The key to be set or deleted
   * @param value If NULL, the key-value pair is deleted, if not NULL then a key-value pair is assigned
   * @return int -1 on exception, any other value otherwise
   */
  static int JSObjectProxy_assign(JSObjectProxy *self, PyObject *key, PyObject *value);

  /**
   * @brief Helper function for various JSObjectProxy methods, sets a key-value pair on a JSObject given a python string key and a JS::Value value
   *
   * @param jsObject - The underlying backing store JSObject for the JSObjectProxy
   * @param key - The key to be assigned or deleted
   * @param value - The JS::Value to be assigned
   */
  static void JSObjectProxy_set_helper(JS::HandleObject jsObject, PyObject *key, JS::HandleValue value);

  /**
   * @brief Comparison method (.tp_richcompare), returns appropriate boolean given a comparison operator and other pyobject
   *
   * @param self - The JSObjectProxy
   * @param other - Any other PyObject
   * @param op - Which boolean operator is being performed (Py_EQ for equality, Py_NE for inequality, all other operators are not implemented)
   * @return PyObject* - True or false depending on result of comparison
   */
  static PyObject *JSObjectProxy_richcompare(JSObjectProxy *self, PyObject *other, int op);

  /**
   * @brief Helper function for JSObjectProxy_richcompare
   *
   * @param self - The PyObject on the left side of the operator (guaranteed to be a JSObjectProxy *)
   * @param other - The PyObject on the right side of the operator
   * @param visited
   * @return bool - Whether the compared objects are equal or not
   */
  static bool JSObjectProxy_richcompare_helper(JSObjectProxy *self, PyObject *other, std::unordered_map<PyObject *, PyObject *> &visited);

  /**
   * @brief Return an iterator object to make JSObjectProxy iterable, emitting (key, value) tuples
   *
   * @param self - The JSObjectProxy
   * @return PyObject* - iterator object
   */
  static PyObject *JSObjectProxy_iter(JSObjectProxy *self);

  /**
   * @brief Compute a string representation of the JSObjectProxy
   *
   * @param self - The JSObjectProxy
   * @return the string representation (a PyUnicodeObject) on success, NULL on failure
   */
  static PyObject *JSObjectProxy_repr(JSObjectProxy *self);
};


/**
 * @brief Struct for the methods that define the Mapping protocol
 *
 */
static PyMappingMethods JSObjectProxy_mapping_methods = {
  .mp_length = (lenfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_length,
  .mp_subscript = (binaryfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_get,
  .mp_ass_subscript = (objobjargproc)JSObjectProxyMethodDefinitions::JSObjectProxy_assign
};

/**
 * @brief Struct for the JSObjectProxyType, used by all JSObjectProxy objects
 */
extern PyTypeObject JSObjectProxyType;
