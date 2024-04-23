/**
 * @file JSObjectProxy.hh
 * @author Caleb Aikens (caleb@distributive.network), Tom Tang (xmader@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief JSObjectProxy is a custom C-implemented python type that derives from dict. It acts as a proxy for JSObjects from Spidermonkey, and behaves like a dict would.
 * @date 2023-06-26
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSObjectProxy_
#define PythonMonkey_JSObjectProxy_

#include <jsapi.h>

#include <Python.h>

#include <unordered_map>


/**
 * @brief The typedef for the backing store that will be used by JSObjectProxy objects. All it contains is a pointer to the JSObject
 *
 */
typedef struct {
  PyDictObject dict;
  JS::PersistentRootedObject *jsObject;
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
   * @brief Length method (.mp_length), returns the number of key-value pairs in the JSObject, used by the python len() method
   *
   * @param self - The JSObjectProxy
   * @return Py_ssize_t The length of the JSObjectProxy
   */
  static Py_ssize_t JSObjectProxy_length(JSObjectProxy *self);

  /**
   * @brief Getter method, returns a value from the JSObjectProxy given a key, used by several built-in python methods as well as the . operator
   *
   * @param self - The JSObjectProxy
   * @param key - The key for the value in the JSObjectProxy
   * @return PyObject* NULL on exception, the corresponding value otherwise
   */
  static PyObject *JSObjectProxy_get(JSObjectProxy *self, PyObject *key);

  /**
   * @brief Getter method (.mp_subscript), returns a value from the JSObjectProxy given a key, used by the [] operator
   *
   * @param self - The JSObjectProxy
   * @param key - The key for the value in the JSObjectProxy
   * @return PyObject* NULL on exception, the corresponding value otherwise
   */
  static PyObject *JSObjectProxy_get_subscript(JSObjectProxy *self, PyObject *key);

  /**
   * @brief Test method (.sq_contains), returns whether a key exists, used by the in operator
   *
   * @param self - The JSObjectProxy
   * @param key - The key for the value in the JSObjectProxy
   * @return int 1 if `key` is in dict, 0 if not, and -1 on error
   */
  static int JSObjectProxy_contains(JSObjectProxy *self, PyObject *key);

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
  // private
  static bool JSObjectProxy_richcompare_helper(JSObjectProxy *self, PyObject *other, std::unordered_map<PyObject *, PyObject *> &visited);

  /**
   * @brief Return an iterator object to make JSObjectProxy iterable, emitting (key, value) tuples
   *
   * @param self - The JSObjectProxy
   * @return PyObject* - iterator object
   */
  static PyObject *JSObjectProxy_iter(JSObjectProxy *self);

  /**
   * @brief Implements next operator function
   *
   * @param self - The JSObjectProxy
   * @return PyObject* - call result
   */
  static PyObject *JSObjectProxy_iter_next(JSObjectProxy *self);

  /**
   * @brief Compute a string representation of the JSObjectProxy
   *
   * @param self - The JSObjectProxy
   * @return the string representation (a PyUnicodeObject) on success, NULL on failure
   */
  static PyObject *JSObjectProxy_repr(JSObjectProxy *self);

  /**
   * @brief Set union operation
   *
   * @param self - The JSObjectProxy
   * @param other - The other PyObject to be or'd, expected to be dict or JSObjectProxy
   * @return PyObject* The resulting new dict
   */
  static PyObject *JSObjectProxy_or(JSObjectProxy *self, PyObject *other);

  /**
   * @brief Set union operation, in place
   *
   * @param self - The JSObjectProxy
   * @param other - The other PyObject to be or'd, expected to be dict or JSObjectProxy
   * @return PyObject* The resulting new dict, must be same object as self
   */
  static PyObject *JSObjectProxy_ior(JSObjectProxy *self, PyObject *other);

  /**
   * @brief get method
   *
   * @param self - The JSObjectProxy
   * @param args - arguments to the method
   * @param nargs - number of args to the method
   * @return PyObject* the value for key if first arg key is in the dictionary, else second arg default
   */
  static PyObject *JSObjectProxy_get_method(JSObjectProxy *self, PyObject *const *args, Py_ssize_t nargs);

  /**
   * @brief setdefault method
   *
   * @param self - The JSObjectProxy
   * @param args - arguments to the method
   * @param nargs - number of args to the method
   * @return PyObject* the value for key if first arg key is in the dictionary, else second default
   */
  static PyObject *JSObjectProxy_setdefault_method(JSObjectProxy *self, PyObject *const *args, Py_ssize_t nargs);

  /**
   * @brief pop method
   *
   * @param self - The JSObjectProxy
   * @param args - arguments to the method
   * @param nargs - number of args to the method
   * @return PyObject* If the first arg key is not found, return the second arg default if given; otherwise raise a KeyError
   */
  static PyObject *JSObjectProxy_pop_method(JSObjectProxy *self, PyObject *const *args, Py_ssize_t nargs);

  /**
   * @brief clear method
   *
   * @param self - The JSObjectProxy
   * @return None
   */
  static PyObject *JSObjectProxy_clear_method(JSObjectProxy *self);

  /**
   * @brief copy method
   *
   * @param self - The JSObjectProxy
   * @return PyObject* copy of the dict
   */
  static PyObject *JSObjectProxy_copy_method(JSObjectProxy *self);

  /**
   * @brief update method    update the dict with another dict or iterable
   *
   * @param self - The JSObjectProxy
   * @param args - arguments to the sort method
   * @param kwds - keyword arguments to the sort method (key-value pairs to be updated in the dict)
   * @return None
   */
  static PyObject *JSObjectProxy_update_method(JSObjectProxy *self, PyObject *args, PyObject *kwds);

  /**
   * @brief keys method
   *
   * @param self - The JSObjectProxy
   * @return PyObject* keys of the dict
   */
  static PyObject *JSObjectProxy_keys_method(JSObjectProxy *self);

  /**
   * @brief values method
   *
   * @param self - The JSObjectProxy
   * @return PyObject* values view of the dict
   */
  static PyObject *JSObjectProxy_values_method(JSObjectProxy *self);

  /**
   * @brief items method
   *
   * @param self - The JSObjectProxy
   * @return PyObject* items view of the dict
   */
  static PyObject *JSObjectProxy_items_method(JSObjectProxy *self);

  /**
   * @brief tp_traverse
   *
   * @param self - The JSObjectProxy
   * @param visit - The function to be applied on each element of the object
   * @param arg - The argument to the visit function
   * @return 0 on success
   */
  static int JSObjectProxy_traverse(JSObjectProxy *self, visitproc visit, void *arg);

  /**
   * @brief tp_clear
   *
   * @param self - The JSObjectProxy
   * @return 0 on success
   */
  static int JSObjectProxy_clear(JSObjectProxy *self);
};


// docs for methods, copied from cpython
PyDoc_STRVAR(getitem__doc__,
  "__getitem__($self, key, /)\n--\n\nReturn self[key].");

PyDoc_STRVAR(dict_get__doc__,
  "get($self, key, default=None, /)\n"
  "--\n"
  "\n"
  "Return the value for key if key is in the dictionary, else default.");

PyDoc_STRVAR(dict_setdefault__doc__,
  "setdefault($self, key, default=None, /)\n"
  "--\n"
  "\n"
  "Insert key with a value of default if key is not in the dictionary.\n"
  "\n"
  "Return the value for key if key is in the dictionary, else default.");

PyDoc_STRVAR(dict_pop__doc__,
  "pop($self, key, default=<unrepresentable>, /)\n"
  "--\n"
  "\n"
  "D.pop(k[,d]) -> v, remove specified key and return the corresponding value.\n"
  "\n"
  "If the key is not found, return the default if given; otherwise,\n"
  "raise a KeyError.");

PyDoc_STRVAR(clear__doc__,
  "D.clear() -> None.  Remove all items from D.");

PyDoc_STRVAR(copy__doc__,
  "D.copy() -> a shallow copy of D");

PyDoc_STRVAR(keys__doc__,
  "D.keys() -> a set-like object providing a view on D's keys");
PyDoc_STRVAR(items__doc__,
  "D.items() -> a set-like object providing a view on D's items");
PyDoc_STRVAR(values__doc__,
  "D.values() -> an object providing a view on D's values");

PyDoc_STRVAR(update__doc__,
  "D.update([E, ]**F) -> None.  Update D from dict/iterable E and F.\n\
If E is present and has a .keys() method, then does:  for k in E: D[k] = E[k]\n\
If E is present and lacks a .keys() method, then does:  for k, v in E: D[k] = v\n\
In either case, this is followed by: for k in F:  D[k] = F[k]");

PyDoc_STRVAR(dict_keys__doc__,
  "D.keys() -> a set-like object providing a view on D's keys");
PyDoc_STRVAR(dict_items__doc__,
  "D.items() -> a set-like object providing a view on D's items");
PyDoc_STRVAR(dict_values__doc__,
  "D.values() -> an object providing a view on D's values");

/**
 * @brief Struct for the methods that define the Mapping protocol
 *
 */
static PyMappingMethods JSObjectProxy_mapping_methods = {
  .mp_length = (lenfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_length,
  .mp_subscript = (binaryfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_get_subscript,
  .mp_ass_subscript = (objobjargproc)JSObjectProxyMethodDefinitions::JSObjectProxy_assign
};

/**
 * @brief Struct for the methods that define the Sequence protocol
 *
 */
static PySequenceMethods JSObjectProxy_sequence_methods = {
  .sq_contains = (objobjproc)JSObjectProxyMethodDefinitions::JSObjectProxy_contains
};

static PyNumberMethods JSObjectProxy_number_methods = {
  .nb_or = (binaryfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_or,
  .nb_inplace_or = (binaryfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_ior
};

/**
 * @brief Struct for the other methods
 *
 */
static PyMethodDef JSObjectProxy_methods[] = {
  {"setdefault", (PyCFunction)JSObjectProxyMethodDefinitions::JSObjectProxy_setdefault_method, METH_FASTCALL, dict_setdefault__doc__},
  {"__getitem__", (PyCFunction)JSObjectProxyMethodDefinitions::JSObjectProxy_get, METH_O | METH_COEXIST, getitem__doc__},
  {"get", (PyCFunction)JSObjectProxyMethodDefinitions::JSObjectProxy_get_method, METH_FASTCALL, dict_get__doc__},
  {"pop", (PyCFunction)JSObjectProxyMethodDefinitions::JSObjectProxy_pop_method, METH_FASTCALL, dict_pop__doc__},
  {"clear", (PyCFunction)JSObjectProxyMethodDefinitions::JSObjectProxy_clear_method, METH_NOARGS, clear__doc__},
  {"copy", (PyCFunction)JSObjectProxyMethodDefinitions::JSObjectProxy_copy_method, METH_NOARGS, copy__doc__},
  {"update", (PyCFunction)JSObjectProxyMethodDefinitions::JSObjectProxy_update_method, METH_VARARGS | METH_KEYWORDS, update__doc__},
  {"keys", (PyCFunction)JSObjectProxyMethodDefinitions::JSObjectProxy_keys_method, METH_NOARGS, dict_keys__doc__},
  {"items", (PyCFunction)JSObjectProxyMethodDefinitions::JSObjectProxy_items_method, METH_NOARGS, dict_items__doc__},
  {"values", (PyCFunction)JSObjectProxyMethodDefinitions::JSObjectProxy_values_method, METH_NOARGS, dict_values__doc__},
  {NULL, NULL}                  /* sentinel */
};

/**
 * @brief Struct for the JSObjectProxyType, used by all JSObjectProxy objects
 */
extern PyTypeObject JSObjectProxyType;

#endif