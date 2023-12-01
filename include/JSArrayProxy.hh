/**
 * @file JSArrayProxy.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSArrayProxy is a custom C-implemented python type that derives from list. It acts as a proxy for JSArrays from Spidermonkey, and behaves like a list would.
 * @version 0.1
 * @date 2023-11-22
 *
 * Copyright (c) 2023 Distributive Corp.
 *
 */


#include <jsapi.h>

#include <Python.h>

#include <unordered_map>

/**
 * @brief The typedef for the backing store that will be used by JSArrayProxy objects. All it contains is a pointer to the JSObject
 *
 */
typedef struct {
  PyListObject list;
  JS::RootedObject jsObject;
} JSArrayProxy;

/**
 * @brief This struct is a bundle of methods used by the JSArrayProxy type
 *
 */
struct JSArrayProxyMethodDefinitions {
public:
  /**
   * @brief Deallocation method (.tp_dealloc), removes the reference to the underlying JSObject before freeing the JSArrayProxy
   *
   * @param self - The JSArrayProxy to be free'd
   */
  static void JSArrayProxy_dealloc(JSArrayProxy *self);

  /**
   * @brief New method (.tp_new), creates a new instance of the JSArrayProxy type, exposed as the __new()__ method in python
   *
   * @param type - The type of object to be created, will always be JSArrayProxyType or a derived type
   * @param args - arguments to the __new()__ method, not used
   * @param kwds - keyword arguments to the __new()__ method, not used
   * @return PyObject* - A new instance of JSArrayProxy
   */
  static PyObject *JSArrayProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

  /**
   * @brief Initialization method (.tp_init), initializes a newly created instance of JSArrayProxy. exposed as the __init()__ method in python
   *
   * @param self - The JSArrayProxy to be initialized
   * @param args - arguments to the __init()__ method, expected to be a dict
   * @param kwds - keyword arguments to the __init()__ method, not used
   * @return int - -1 on exception, return any other value otherwise
   */
  static int JSArrayProxy_init(JSArrayProxy *self, PyObject *args, PyObject *kwds);

  /**
   * @brief Length method (.mp_length and .sq_length), returns the number of key-value pairs in the JSObject, used by the python len() method
   *
   * @param self - The JSArrayProxy
   * @return Py_ssize_t The length of the JSArrayProxy
   */
  static Py_ssize_t JSArrayProxy_length(JSArrayProxy *self);

  /**
   * @brief Getter method (.mp_subscript), returns a value from the JSArrayProxy given a key, used by several built-in python methods as well as the [] operator
   *
   * @param self - The JSArrayProxy
   * @param key - The key for the value in the JSArrayProxy
   * @return PyObject* NULL on exception, the corresponding value otherwise
   */
  static PyObject *JSArrayProxy_get(JSArrayProxy *self, PyObject *key);


  /**
   * @brief Assign method (.mp_ass_subscript), assigns a key-value pair if value is non-NULL, or deletes a key-value pair if value is NULL
   *
   * @param self - The JSArrayProxy
   * @param key - The key to be set or deleted
   * @param value If NULL, the key-value pair is deleted, if not NULL then a key-value pair is assigned
   * @return int -1 on exception, any other value otherwise
   */
  static int JSArrayProxy_assign_key(JSArrayProxy *self, PyObject *key, PyObject *value);

  /**
   * @brief Helper function for various JSArrayProxy methods, sets a key-value pair on a JSObject given a python string key and a JS::Value value
   *
   * @param jsObject - The underlying backing store JSObject for the JSArrayProxy
   * @param key - The key to be assigned or deleted
   * @param value - The JS::Value to be assigned
   */
  static void JSArrayProxy_set_helper(JS::HandleObject jsObject, PyObject *key, JS::HandleValue value);

  /**
   * @brief Comparison method (.tp_richcompare), returns appropriate boolean given a comparison operator and other pyobject
   *
   * @param self - The JSArrayProxy
   * @param other - Any other PyObject
   * @param op - Which boolean operator is being performed (Py_EQ for equality, Py_NE for inequality, all other operators are not implemented)
   * @return PyObject* - True or false depending on result of comparison
   */
  static PyObject *JSArrayProxy_richcompare(JSArrayProxy *self, PyObject *other, int op);

  /**
   * @brief Helper function for JSArrayProxy_richcompare
   *
   * @param self - The PyObject on the left side of the operator (guaranteed to be a JSArrayProxy *)
   * @param other - The PyObject on the right side of the operator
   * @param visited
   * @return bool - Whether the compared objects are equal or not
   */
  static bool JSArrayProxy_richcompare_helper(JSArrayProxy *self, PyObject *other, std::unordered_map<PyObject *, PyObject *> &visited);

  /**
   * @brief Return an iterator object to make JSArrayProxy iterable, emitting (key, value) tuples
   *
   * @param self - The JSArrayProxy
   * @return PyObject* - iterator object
   */
  static PyObject *JSArrayProxy_iter(JSArrayProxy *self);

  /**
   * @brief Compute a string representation of the JSArrayProxy
   *
   * @param self - The JSArrayProxy
   * @return the string representation (a PyUnicodeObject) on success, NULL on failure
   */
  static PyObject *JSArrayProxy_repr(JSArrayProxy *self);

  /**
   * @brief concat method (.sq_concat), concatenates
   *
   * @param self - The JSArrayProxy
   * @param value - The value to be concatenated
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  // static PyObject *JSArrayProxy_concat(JSArrayProxy *self, PyObject *value);

  /**
   * @brief repeat method (.sq_repeat)
   *
   * @param self - The JSArrayProxy
   * @param n The number of times to repeat
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
//  static PyObject *JSArrayProxy_repeat(JSArrayProxy *self, Py_ssize_t n);

  /**
   * @brief Getter method (.sq_item), returns a value from the JSArrayProxy given an index, used by several built-in python methods as well as the [] operator
   *
   * @param self - The JSArrayProxy
   * @param index - The index for the value in the JSArrayProxy
   * @return PyObject* NULL on exception, the corresponding value otherwise
   */
  // static PyObject *JSArrayProxy_item(JSArrayProxy *self, Py_ssize_t index);

  /**
   * @brief Assign method (.sq_ass_item), assigns a value at index if value is non-NULL, or deletes a key-value pair if value is NULL
   *
   * @param self - The JSObjectProxy
   * @param index - The index for the value in the JSArrayProxy
   * @param value If NULL, the key-value pair is deleted, if not NULL then a key-value pair is assigned
   * @return int -1 on exception, a0 on success
   */
  // static int JSArrayProxy_assign_index(JSArrayProxy *self, Py_ssize_t index, PyObject *value);

  /**
   * @brief Test contains method (.sq_contains), assigns a value at index if value is non-NULL, or deletes a key-value pair if value is NULL
   *
   * @param self - The JSObjectProxy
   * @param element - The element in the JSArrayProxy
   * @return int 1 if element is in List, 0 if not, and -1 on error
   */
  // static int JSArrayProxy_contains(JSArrayProxy *self, PyObject *element);

  /**
   * @brief inplace_concat method (.sq_inplace_concat), concatenates
   *
   * @param self - The JSArrayProxy
   * @param value - The value to be concatenated
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  // static PyObject *JSArrayProxy_inplace_concat(JSArrayProxy *self, PyObject *value);

  /**
   * @brief inplace_repeat method (.sq_inplace_repeat)
   *
   * @param self - The JSArrayProxy
   * @param n The number of times to repeat
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  // static PyObject *JSArrayProxy_inplace_repeat(JSArrayProxy *self, Py_ssize_t n);

  /**
   * @brief clear method (.tp_clear)
   *
   * @param self - The JSArrayProxy
   * @return 0 on success
   */
  // static int JSArrayProxy_clear(JSArrayProxy *self);

  /**
   * @brief copy method
   *
   * @param self - The JSArrayProxy
   * @return a shallow copy of the list
   */
  // static PyObject *JSArrayProxy_copy(JSArrayProxy *self);


  /**
   * @brief append method
   *
   * @param self - The JSArrayProxy
   * @param value - The value to be appended
   * @return None
   */
  // static PyObject *JSArrayProxy_append(JSArrayProxy *self, PyObject *value);

  /**
   * @brief insert method
   *
   * @param self - The JSArrayProxy
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  // static PyObject *JSArrayProxy_insert(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs);

  /**
   * @brief extend method
   *
   * @param self - The JSArrayProxy
   * @param value - The value to be appended
   * @return None
   */
  // static PyObject *JSArrayProxy_extend(JSArrayProxy *self, PyObject *iterable);

  /**
   * @brief pop method
   *
   * @param self - The JSArrayProxy
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  // static PyObject *JSArrayProxy_pop(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs);


  /**
   * @brief remove method   Remove first occurrence of value
   *
   * @param self - The JSArrayProxy
   * @param value - The value to be appended
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  // static PyObject *JSArrayProxy_remove(JSArrayProxy *self, PyObject *value);

  /**
   * @brief index method
   *
   * @param self - The JSArrayProxy
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  // static PyObject *JSArrayProxy_index(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs);

  /**
   * @brief count method   Remove first occurrence of value
   *
   * @param self - The JSArrayProxy
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  // static PyObject *JSArrayProxy_count(JSArrayProxy *self, PyObject *value);

  /**
   * @brief reverse method   Reverse list in place
   *
   * @param self - The JSArrayProxy
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  // static PyObject *JSArrayProxy_reverse(JSArrayProxy *self);


  /**
   * @brief sort method   sort in place
   *
   * @param self - The JSArrayProxy
   * @param value - The value to be appended
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  // static PyObject *JSArrayProxy_sort(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);
};


/**
 * @brief Struct for the methods that define the Mapping protocol
 *
 */
static PyMappingMethods JSArrayProxy_mapping_methods = {
  .mp_length = (lenfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_length,
  .mp_subscript = (binaryfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_get,
  .mp_ass_subscript = (objobjargproc)JSArrayProxyMethodDefinitions::JSArrayProxy_assign_key
};

/**
 * @brief Struct for the methods that define the Sequence protocol
 *
 */
static PySequenceMethods JSArrayProxy_sequence_methods = {
  .sq_length = (lenfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_length,
  /*.sq_concat = (binaryfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_concat,
     .sq_repeat = (ssizeargfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_repeat,
     .sq_item = (ssizeargfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_item,
     .sq_ass_item = (ssizeobjargproc)JSArrayProxyMethodDefinitions::JSArrayProxy_assign_index,
     .sq_contains = (objobjproc)JSArrayProxyMethodDefinitions::JSArrayProxy_contains,
     .sq_inplace_concat = (binaryfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_inplace_concat,
     .sq_inplace_repeat = (ssizeargfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_inplace_repeat,*/
};

/**
 * @brief Struct for the other methods
 *
 */
static PyMethodDef JSArrayProxy_methods[] = {
  // {"__getitem__", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_get, METH_O|METH_COEXIST,
  // PyDoc_STR("__getitem__($self, index, /)\n--\n\nReturn self[index].")},
  // {"clear", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_clear, METH_NOARGS, ""},
  // {"copy", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_copy, METH_NOARGS, ""},
  // {"append", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_append, METH_O, ""},
  // {"insert", _PyCFunction_CAST(JSArrayProxyMethodDefinitions::JSArrayProxy_insert), METH_FASTCALL, ""},
  // {"extend", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_extend, METH_O, ""},
  // {"pop", _PyCFunction_CAST(JSArrayProxyMethodDefinitions::JSArrayProxy_pop), METH_FASTCALL, ""}, // TODO can empty string be null?
  // {"remove", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_remove, METH_O, ""},
  // {"index", _PyCFunction_CAST(JSArrayProxyMethodDefinitions::JSArrayProxy_remove), METH_FASTCALL, ""},
  // {"count", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_remove, METH_O, ""},
  // {"reverse", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_reverse, METH_NOARGS, ""},
  // {"sort", _PyCFunction_CAST(JSArrayProxyMethodDefinitions::JSArrayProxy_sort), METH_FASTCALL|METH_KEYWORDS, ""},
  {NULL, NULL}                       /* sentinel */
};

/**
 * @brief Struct for the JSArrayProxyType, used by all JSArrayProxy objects
 */
extern PyTypeObject JSArrayProxyType;
