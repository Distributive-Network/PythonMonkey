/**
 * @file JSArrayProxy.hh
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSArrayProxy is a custom C-implemented python type that derives from list. It acts as a proxy for JSArrays from Spidermonkey, and behaves like a list would.
 * @version 0.1
 * @date 2023-11-22
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_JSArrayProxy_
#define PythonMonkey_JSArrayProxy_


#include <jsapi.h>

#include <Python.h>


/**
 * @brief The typedef for the backing store that will be used by JSArrayProxy objects. All it contains is a pointer to the JSObject
 *
 */
typedef struct {
  PyListObject list;
  JS::PersistentRootedObject *jsArray;
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
   * @brief Length method (.mp_length and .sq_length), returns the number of keys in the JSObject, used by the python len() method
   *
   * @param self - The JSArrayProxy
   * @return Py_ssize_t The length of the JSArrayProxy
   */
  static Py_ssize_t JSArrayProxy_length(JSArrayProxy *self);

  /**
   * @brief returns a value from the JSArrayProxy given a key, or dispatches to the given key method if such method is found
   *
   * @param self - The JSArrayProxy
   * @param key - The key for the value in the JSArrayProxy
   * @return PyObject* NULL on exception, the corresponding value otherwise
   */
  static PyObject *JSArrayProxy_get(JSArrayProxy *self, PyObject *key);


  /**
   * @brief Getter method (.mp_subscript), returns a value from the JSArrayProxy given a key which can be a slice, used by several built-in python methods as well as the [] and operator
   *
   * @param self - The JSArrayProxy
   * @param key - The key for the value in the JSArrayProxy
   * @return PyObject* NULL on exception, the corresponding value otherwise
   */
  static PyObject *JSArrayProxy_get_subscript(JSArrayProxy *self, PyObject *key);

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
   * @brief Comparison method (.tp_richcompare), returns appropriate boolean given a comparison operator and other pyObject
   *
   * @param self - The JSArrayProxy
   * @param other - Any other PyObject
   * @param op - Which boolean operator is being performed (Py_EQ for equality, Py_NE for inequality, all other operators are not implemented)
   * @return PyObject* - True or false depending on result of comparison
   */
  static PyObject *JSArrayProxy_richcompare(JSArrayProxy *self, PyObject *other, int op);

  /**
   * @brief Return an iterator object to make JSArrayProxy iterable
   *
   * @param self - The JSArrayProxy
   * @return PyObject* - iterator object
   */
  static PyObject *JSArrayProxy_iter(JSArrayProxy *self);

  /**
   * @brief Return a reverse iterator object to make JSArrayProxy backwards iterable
   *
   * @param self - The JSArrayProxy
   * @return PyObject* - iterator object
   */
  static PyObject *JSArrayProxy_iter_reverse(JSArrayProxy *self);

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
  static PyObject *JSArrayProxy_concat(JSArrayProxy *self, PyObject *value);

  /**
   * @brief repeat method (.sq_repeat), repeat self n number of time
   *
   * @param self - The JSArrayProxy
   * @param n The number of times to repeat
   * @return PyObject* NULL on exception, the corresponding new value otherwise
   */
  static PyObject *JSArrayProxy_repeat(JSArrayProxy *self, Py_ssize_t n);

  /**
   * @brief Test contains method (.sq_contains)
   *
   * @param self - The JSObjectProxy
   * @param element - The element in the JSArrayProxy
   * @return int 1 if element is in List, 0 if not, and -1 on error
   */
  static int JSArrayProxy_contains(JSArrayProxy *self, PyObject *element);

  /**
   * @brief inplace_concat method (.sq_inplace_concat), concatenates in_place
   *
   * @param self - The JSArrayProxy
   * @param value - The value to be concatenated
   * @return PyObject* self
   */
  static PyObject *JSArrayProxy_inplace_concat(JSArrayProxy *self, PyObject *value);

  /**
   * @brief inplace_repeat method (.sq_inplace_repeat), repeats in_place
   *
   * @param self - The JSArrayProxy
   * @param n The number of times to repeat
   * @return PyObject* self
   */
  static PyObject *JSArrayProxy_inplace_repeat(JSArrayProxy *self, Py_ssize_t n);

  /**
   * @brief clear method, empties the array
   *
   * @param self - The JSArrayProxy
   * @return None
   */
  static PyObject *JSArrayProxy_clear_method(JSArrayProxy *self);

  /**
   * @brief copy method
   *
   * @param self - The JSArrayProxy
   * @return a shallow copy of the list
   */
  static PyObject *JSArrayProxy_copy(JSArrayProxy *self);

  /**
   * @brief append method
   *
   * @param self - The JSArrayProxy
   * @param value - The value to be appended
   * @return PyObject* NULL on exception, None otherwise
   */
  static PyObject *JSArrayProxy_append(JSArrayProxy *self, PyObject *value);

  /**
   * @brief insert method
   *
   * @param self - The JSArrayProxy
   * @param args - arguments to the insert method
   * @param nargs - number of arguments to the insert method
   * @return PyObject* NULL on exception, None otherwise
   */
  static PyObject *JSArrayProxy_insert(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs);

  /**
   * @brief extend method
   *
   * @param self - The JSArrayProxy
   * @param iterable - The value to be appended
   * @return PyObject* NULL on exception, None otherwise
   */
  static PyObject *JSArrayProxy_extend(JSArrayProxy *self, PyObject *iterable);

  /**
   * @brief pop method
   *
   * @param self - The JSArrayProxy
   * @param args - arguments to the pop method
   * @param nargs - number of arguments to the pop method
   * @return PyObject* NULL on exception, the corresponding value otherwise
   */
  static PyObject *JSArrayProxy_pop(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs);

  /**
   * @brief remove method   Remove first occurrence of value
   *
   * @param self - The JSArrayProxy
   * @param value - The value to be appended
   * @return PyObject* NULL on exception, None otherwise
   */
  static PyObject *JSArrayProxy_remove(JSArrayProxy *self, PyObject *value);

  /**
   * @brief index method
   *
   * @param self - The JSArrayProxy
   * @param args - arguments to the index method
   * @param nargs - number of arguments to the index method
   * @return PyObject* NULL on exception, the corresponding index of the found value as PyLong otherwise
   */
  static PyObject *JSArrayProxy_index(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs);

  /**
   * @brief count method
   *
   * @param self - The JSArrayProxy
   * @param value - The value to be appended
   * @return PyObject* NULL on exception, the corresponding count of the found value as PyLong otherwise
   */
  static PyObject *JSArrayProxy_count(JSArrayProxy *self, PyObject *value);

  /**
   * @brief reverse method   Reverse list in place
   *
   * @param self - The JSArrayProxy
   * @return PyObject* NULL on exception, None otherwise
   */
  static PyObject *JSArrayProxy_reverse(JSArrayProxy *self);

  /**
   * @brief sort method   sort in place
   *
   * @param self - The JSArrayProxy
   * @param args - arguments to the sort method (not used)
   * @param nargs - number of arguments to the sort method
   * @param kwnames - keyword arguments to the sort method (reverse=True|False, key=keyfunction)
   * @return PyObject* NULL on exception, None otherwise
   */
  static PyObject *JSArrayProxy_sort(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

  /**
   * @brief tp_traverse
   *
   * @param self - The JSArrayProxy
   * @param visit - The function to be applied on each element of the list
   * @param arg - The argument to the visit function
   * @return 0 on success
   */
  static int JSArrayProxy_traverse(JSArrayProxy *self, visitproc visit, void *arg);

  /**
   * @brief tp_clear
   *
   * @param self - The JSArrayProxy
   * @return 0 on success
   */
  static int JSArrayProxy_clear(JSArrayProxy *self);
};


/**
 * @brief Struct for the methods that define the Mapping protocol
 *
 */
static PyMappingMethods JSArrayProxy_mapping_methods = {
  .mp_length = (lenfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_length,
  .mp_subscript = (binaryfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_get_subscript,
  .mp_ass_subscript = (objobjargproc)JSArrayProxyMethodDefinitions::JSArrayProxy_assign_key
};

/**
 * @brief Struct for the methods that define the Sequence protocol
 *
 */
static PySequenceMethods JSArrayProxy_sequence_methods = {
  .sq_length = (lenfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_length,
  .sq_concat = (binaryfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_concat,
  .sq_repeat = (ssizeargfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_repeat,
  .sq_contains = (objobjproc)JSArrayProxyMethodDefinitions::JSArrayProxy_contains,
  .sq_inplace_concat = (binaryfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_inplace_concat,
  .sq_inplace_repeat = (ssizeargfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_inplace_repeat
};


PyDoc_STRVAR(py_list_clear__doc__,
  "clear($self, /)\n"
  "--\n"
  "\n"
  "Remove all items from list.");

PyDoc_STRVAR(list_copy__doc__,
  "copy($self, /)\n"
  "--\n"
  "\n"
  "Return a shallow copy of the list.");

PyDoc_STRVAR(list_append__doc__,
  "append($self, object, /)\n"
  "--\n"
  "\n"
  "Append object to the end of the list.");

PyDoc_STRVAR(list_insert__doc__,
  "insert($self, index, object, /)\n"
  "--\n"
  "\n"
  "Insert object before index.");

PyDoc_STRVAR(py_list_extend__doc__,
  "extend($self, iterable, /)\n"
  "--\n"
  "\n"
  "Extend list by appending elements from the iterable.");

PyDoc_STRVAR(list_pop__doc__,
  "pop($self, index=-1, /)\n"
  "--\n"
  "\n"
  "Remove and return item at index (default last).\n"
  "\n"
  "Raises IndexError if list is empty or index is out of range.");

PyDoc_STRVAR(list_remove__doc__,
  "remove($self, value, /)\n"
  "--\n"
  "\n"
  "Remove first occurrence of value.\n"
  "\n"
  "Raises ValueError if the value is not present.");

PyDoc_STRVAR(list_index__doc__,
  "index($self, value, start=0, stop=sys.maxsize, /)\n"
  "--\n"
  "\n"
  "Return first index of value.\n"
  "\n"
  "Raises ValueError if the value is not present.");


PyDoc_STRVAR(list_count__doc__,
  "count($self, value, /)\n"
  "--\n"
  "\n"
  "Return number of occurrences of value.");

PyDoc_STRVAR(list_reverse__doc__,
  "reverse($self, /)\n"
  "--\n"
  "\n"
  "Reverse *IN PLACE*.");

PyDoc_STRVAR(list_sort__doc__,
  "sort($self, /, *, key=None, reverse=False)\n"
  "--\n"
  "\n"
  "Sort the list in ascending order and return None.\n"
  "\n"
  "The sort is in-place (i.e. the list itself is modified) and stable (i.e. the\n"
  "order of two equal elements is maintained).\n"
  "\n"
  "If a key function is given, apply it once to each list item and sort them,\n"
  "ascending or descending, according to their function values.\n"
  "\n"
  "The reverse flag can be set to sort in descending order.");

PyDoc_STRVAR(list___reversed____doc__,
  "__reversed__($self, /)\n"
  "--\n"
  "\n"
  "Return a reverse iterator over the list.");

/**
 * @brief Struct for the other methods
 *
 */
static PyMethodDef JSArrayProxy_methods[] = {
  {"__reversed__", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_iter_reverse, METH_NOARGS, list___reversed____doc__},
  {"clear", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_clear_method, METH_NOARGS, py_list_clear__doc__},
  {"copy", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_copy, METH_NOARGS, list_copy__doc__},
  {"append", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_append, METH_O, list_append__doc__},
  {"insert", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_insert, METH_FASTCALL, list_insert__doc__},
  {"extend", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_extend, METH_O, py_list_extend__doc__},
  {"pop", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_pop, METH_FASTCALL, list_pop__doc__},
  {"remove", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_remove, METH_O, list_remove__doc__},
  {"index", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_index, METH_FASTCALL, list_index__doc__},
  {"count", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_count, METH_O, list_count__doc__},
  {"reverse", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_reverse, METH_NOARGS, list_reverse__doc__},
  {"sort", (PyCFunction)JSArrayProxyMethodDefinitions::JSArrayProxy_sort, METH_FASTCALL|METH_KEYWORDS, list_sort__doc__},
  {NULL, NULL}                       /* sentinel */
};

/**
 * @brief Struct for the JSArrayProxyType, used by all JSArrayProxy objects
 */
extern PyTypeObject JSArrayProxyType;

#endif