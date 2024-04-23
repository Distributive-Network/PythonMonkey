/**
 * @file JSArrayProxy.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSArrayProxy is a custom C-implemented python type that derives from list. It acts as a proxy for JSArrays from Spidermonkey, and behaves like a list would.
 * @date 2023-11-22
 *
 * @copyright Copyright (c) 2023-2024 Distributive Corp.
 *
 */


#include "include/JSArrayProxy.hh"

#include "include/JSArrayIterProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/PyBaseProxyHandler.hh"
#include "include/JSFunctionProxy.hh"

#include <jsapi.h>
#include <jsfriendapi.h>

#include <Python.h>


void JSArrayProxyMethodDefinitions::JSArrayProxy_dealloc(JSArrayProxy *self)
{
  self->jsArray->set(nullptr);
  delete self->jsArray;
  PyObject_GC_UnTrack(self);
  PyObject_GC_Del(self);
}

int JSArrayProxyMethodDefinitions::JSArrayProxy_traverse(JSArrayProxy *self, visitproc visit, void *arg)
{
  // Nothing to be done
  return 0;
}

int JSArrayProxyMethodDefinitions::JSArrayProxy_clear(JSArrayProxy *self)
{
  // Nothing to be done
  return 0;
}

Py_ssize_t JSArrayProxyMethodDefinitions::JSArrayProxy_length(JSArrayProxy *self)
{
  uint32_t length;
  JS::GetArrayLength(GLOBAL_CX, *(self->jsArray), &length);
  return (Py_ssize_t)length;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_get(JSArrayProxy *self, PyObject *key)
{
  JS::RootedId id(GLOBAL_CX);
  if (!keyToId(key, &id)) {
    PyErr_SetString(PyExc_AttributeError, "JSArrayProxy property name must be of type str or int");
    return NULL;
  }

  // look through the methods for dispatch and return key if no method found
  for (size_t index = 0;; index++) {
    const char *methodName = JSArrayProxyType.tp_methods[index].ml_name;
    if (methodName == NULL || !PyUnicode_Check(key)) {   // reached end of list
      JS::RootedValue value(GLOBAL_CX);
      JS_GetPropertyById(GLOBAL_CX, *(self->jsArray), id, &value);
      if (value.isUndefined() && PyUnicode_Check(key)) {
        if (strcmp("__class__", PyUnicode_AsUTF8(key)) == 0) {
          return PyObject_GenericGetAttr((PyObject *)self, key);
        }
      }
      return pyTypeFactory(GLOBAL_CX, value);
    }
    else {
      if (strcmp(methodName, PyUnicode_AsUTF8(key)) == 0) {
        return PyObject_GenericGetAttr((PyObject *)self, key);
      }
    }
  }
}

// private
static PyObject *list_slice(JSArrayProxy *self, Py_ssize_t ilow, Py_ssize_t ihigh)
{
  JS::Rooted<JS::ValueArray<2>> jArgs(GLOBAL_CX);
  jArgs[0].setInt32(ilow);
  jArgs[1].setInt32(ihigh);
  JS::RootedValue jReturnedArray(GLOBAL_CX);
  if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "slice", jArgs, &jReturnedArray)) {
    PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
    return NULL;
  }
  return pyTypeFactory(GLOBAL_CX, jReturnedArray);
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_get_subscript(JSArrayProxy *self, PyObject *key)
{
  if (PyIndex_Check(key)) {
    Py_ssize_t index = PyNumber_AsSsize_t(key, PyExc_IndexError);
    if (index == -1 && PyErr_Occurred()) {
      return NULL;
    }

    Py_ssize_t selfLength = JSArrayProxy_length(self);

    if (index < 0) {
      index += selfLength;
    }

    if ((size_t)index >= (size_t)selfLength) {
      PyErr_SetObject(PyExc_IndexError, PyUnicode_FromString("list index out of range"));
      return NULL;
    }

    JS::RootedId id(GLOBAL_CX);
    JS_IndexToId(GLOBAL_CX, index, &id);

    JS::RootedValue value(GLOBAL_CX);
    JS_GetPropertyById(GLOBAL_CX, *(self->jsArray), id, &value);

    return pyTypeFactory(GLOBAL_CX, value);
  }
  else if (PySlice_Check(key)) {
    Py_ssize_t start, stop, step, slicelength, index;

    if (PySlice_Unpack(key, &start, &stop, &step) < 0) {
      return NULL;
    }

    slicelength = PySlice_AdjustIndices(JSArrayProxy_length(self), &start, &stop, step);

    if (slicelength <= 0) {
      return PyList_New(0);
    }
    else if (step == 1) {
      return list_slice(self, start, stop);
    }
    else {
      JS::RootedObject jCombinedArray(GLOBAL_CX, JS::NewArrayObject(GLOBAL_CX, slicelength));

      JS::RootedValue elementVal(GLOBAL_CX);
      for (size_t cur = start, index = 0; index < slicelength; cur += (size_t)step, index++) {
        JS_GetElement(GLOBAL_CX, *(self->jsArray), cur, &elementVal);
        JS_SetElement(GLOBAL_CX, jCombinedArray, index, elementVal);
      }

      JS::RootedValue jCombinedArrayValue(GLOBAL_CX);
      jCombinedArrayValue.setObjectOrNull(jCombinedArray);

      return pyTypeFactory(GLOBAL_CX, jCombinedArrayValue);
    }
  }
  else {
    PyErr_Format(PyExc_TypeError, "list indices must be integers or slices, not %.200s", Py_TYPE(key)->tp_name);
    return NULL;
  }
}

/* a[ilow:ihigh] = v if v != NULL.
 * del a[ilow:ihigh] if v == NULL.
 */
// private
static int list_ass_slice(JSArrayProxy *self, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
  /* Because [X]DECREF can recursively invoke list operations on
     this list, we must postpone all [X]DECREF activity until
     after the list is back in its canonical shape.  Therefore
     we must allocate an additional array, 'recycle', into which
     we temporarily copy the items that are deleted from the
     list. :-( */
  PyObject **item;
  PyObject **vitem = NULL;
  PyObject *v_as_SF = NULL;   /* PySequence_Fast(v) */
  Py_ssize_t n;   /* # of elements in replacement list */
  Py_ssize_t norig;   /* # of elements in list getting replaced */
  Py_ssize_t d;   /* Change in size */
  Py_ssize_t k;
  size_t s;
  int result = -1;              /* guilty until proved innocent */
#define b ((PyListObject *)v)
  Py_ssize_t selfLength = JSArrayProxyMethodDefinitions::JSArrayProxy_length(self);
  if (v == NULL) {
    n = 0;
  }
  else {
    if ((PyListObject *)self == b) {
      /* Special case "a[i:j] = a" -- copy b first */
      v = list_slice(self, 0, selfLength);
      if (v == NULL) {
        return result;
      }
      result = list_ass_slice(self, ilow, ihigh, v);
      Py_DECREF(v);
      return result;
    }
    v_as_SF = PySequence_Fast(v, "can only assign an iterable");
    if (v_as_SF == NULL) {
      return result;
    }
    n = PySequence_Fast_GET_SIZE(v_as_SF);
    vitem = PySequence_Fast_ITEMS(v_as_SF);
  }

  if (ilow < 0) {
    ilow = 0;
  }
  else if (ilow > selfLength) {
    ilow = selfLength;
  }

  if (ihigh < ilow) {
    ihigh = ilow;
  }
  else if (ihigh > selfLength) {
    ihigh = selfLength;
  }

  norig = ihigh - ilow;
  assert(norig >= 0);
  d = n - norig;

  if (selfLength + d == 0) {
    Py_XDECREF(v_as_SF);
    JSArrayProxyMethodDefinitions::JSArrayProxy_clear_method(self);
    return 0;
  }

  if (d < 0) {   /* Delete -d items */
    JS::RootedValue elementVal(GLOBAL_CX);
    for (size_t index = ihigh, count = 0; count < selfLength - ihigh; index++, count++) {
      JS_GetElement(GLOBAL_CX, *(self->jsArray), index, &elementVal);
      JS_SetElement(GLOBAL_CX, *(self->jsArray), index+d, elementVal);
    }

    JS::SetArrayLength(GLOBAL_CX, *(self->jsArray), selfLength + d);
  }
  else if (d > 0) { /* Insert d items */
    k = selfLength;

    JS::SetArrayLength(GLOBAL_CX, *(self->jsArray), k + d);

    selfLength = k + d;

    JS::RootedValue elementVal(GLOBAL_CX);
    for (size_t index = ihigh, count = 0; count < k - ihigh; index++, count++) {
      JS_GetElement(GLOBAL_CX, *(self->jsArray), index, &elementVal);
      JS_SetElement(GLOBAL_CX, *(self->jsArray), index+d, elementVal);
    }
  }

  JS::RootedValue elementVal(GLOBAL_CX);
  for (k = 0; k < n; k++, ilow++) {
    elementVal.set(jsTypeFactory(GLOBAL_CX, vitem[k]));
    JS_SetElement(GLOBAL_CX, *(self->jsArray), ilow, elementVal);
  }

  result = 0;
  Py_XDECREF(v_as_SF);
  return result;
#undef b
}

int JSArrayProxyMethodDefinitions::JSArrayProxy_assign_key(JSArrayProxy *self, PyObject *key, PyObject *value)
{
  if (PyIndex_Check(key)) {
    Py_ssize_t index = PyNumber_AsSsize_t(key, PyExc_IndexError);
    if (index == -1 && PyErr_Occurred()) {
      return -1;
    }

    Py_ssize_t selfLength = JSArrayProxy_length(self);

    if (index < 0) {
      index += selfLength;
    }

    if ((size_t)index >= (size_t)selfLength) {
      PyErr_SetObject(PyExc_IndexError, PyUnicode_FromString("list assignment index out of range"));
      return -1;
    }

    JS::RootedId id(GLOBAL_CX);
    JS_IndexToId(GLOBAL_CX, index, &id);

    if (value) { // we are setting a value
      JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, value));
      JS_SetPropertyById(GLOBAL_CX, *(self->jsArray), id, jValue);
    } else { // we are deleting a value
      JS::ObjectOpResult ignoredResult;
      JS_DeletePropertyById(GLOBAL_CX, *(self->jsArray), id, ignoredResult);
    }

    return 0;
  }
  else if (PySlice_Check(key)) {
    Py_ssize_t start, stop, step, slicelength;

    if (PySlice_Unpack(key, &start, &stop, &step) < 0) {
      return -1;
    }

    Py_ssize_t selfSize = JSArrayProxy_length(self);

    slicelength = PySlice_AdjustIndices(selfSize, &start, &stop, step);

    if (step == 1) {
      return list_ass_slice(self, start, stop, value);
    }

    /* Make sure s[5:2] = [..] inserts at the right place:
       before 5, not before 2. */
    if ((step < 0 && start < stop) || (step > 0 && start > stop)) {
      stop = start;
    }

    if (value == NULL) {
      /* delete slice */
      size_t cur;
      Py_ssize_t i;

      if (slicelength <= 0) {
        return 0;
      }

      if (step < 0) {
        stop = start + 1;
        start = stop + step*(slicelength - 1) - 1;
        step = -step;
      }

      /* drawing pictures might help understand these for
         loops. Basically, we memmove the parts of the
         list that are *not* part of the slice: step-1
         items for each item that is part of the slice,
         and then tail end of the list that was not
         covered by the slice */
      JS::RootedValue elementVal(GLOBAL_CX);
      for (cur = start, i = 0; cur < (size_t)stop; cur += step, i++) {
        Py_ssize_t lim = step - 1;

        if (cur + step >= (size_t)selfSize) {
          lim = selfSize - cur - 1;
        }

        for (size_t index = cur, count = 0; count < lim; index++, count++) {
          JS_GetElement(GLOBAL_CX, *(self->jsArray), index + 1, &elementVal);
          JS_SetElement(GLOBAL_CX, *(self->jsArray), index - i, elementVal);
        }
      }

      cur = start + (size_t)slicelength * step;

      if (cur < (size_t)selfSize) {
        for (size_t index = cur, count = 0; count < selfSize - cur; index++, count++) {
          JS_GetElement(GLOBAL_CX, *(self->jsArray), index, &elementVal);
          JS_SetElement(GLOBAL_CX, *(self->jsArray), index - slicelength, elementVal);
        }
      }

      JS::SetArrayLength(GLOBAL_CX, *(self->jsArray), selfSize - slicelength);

      return 0;
    }
    else {
      /* assign slice */
      PyObject *ins, *seq;
      PyObject **seqitems, **selfitems;
      Py_ssize_t i;
      size_t cur;

      /* protect against a[::-1] = a */
      if ((PyListObject *)self == (PyListObject *)value) {
        seq = list_slice((JSArrayProxy *)value, 0, JSArrayProxy_length((JSArrayProxy *)value));
      }
      else {
        seq = PySequence_Fast(value, "must assign iterable to extended slice");
      }

      if (!seq) {
        return -1;
      }

      if (PySequence_Fast_GET_SIZE(seq) != slicelength) {
        PyErr_Format(PyExc_ValueError, "attempt to assign sequence of size %zd to extended slice of size %zd",
          PySequence_Fast_GET_SIZE(seq), slicelength);
        Py_DECREF(seq);
        return -1;
      }

      if (!slicelength) {
        Py_DECREF(seq);
        return 0;
      }

      seqitems = PySequence_Fast_ITEMS(seq);

      JS::RootedValue elementVal(GLOBAL_CX);
      for (cur = start, i = 0; i < slicelength; cur += (size_t)step, i++) {
        elementVal.set(jsTypeFactory(GLOBAL_CX, seqitems[i]));
        JS_SetElement(GLOBAL_CX, *(self->jsArray), cur, elementVal);
      }

      Py_DECREF(seq);

      return 0;
    }
  }
  else {
    PyErr_Format(PyExc_TypeError, "list indices must be integers or slices, not %.200s", Py_TYPE(key)->tp_name);
    return -1;
  }
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_richcompare(JSArrayProxy *self, PyObject *other, int op)
{
  if (!PyList_Check(self) || !PyList_Check(other)) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  if (self == (JSArrayProxy *)other && (op == Py_EQ || op == Py_NE)) {
    if (op == Py_EQ) {
      Py_RETURN_TRUE;
    }
    else {
      Py_RETURN_FALSE;
    }
  }

  Py_ssize_t selfLength = JSArrayProxy_length(self);
  Py_ssize_t otherLength;

  if (PyObject_TypeCheck(other, &JSArrayProxyType)) {
    otherLength = JSArrayProxy_length((JSArrayProxy *)other);
  } else {
    otherLength = Py_SIZE(other);
  }

  if (selfLength != otherLength && (op == Py_EQ || op == Py_NE)) {
    /* Shortcut: if the lengths differ, the lists differ */
    if (op == Py_EQ) {
      Py_RETURN_FALSE;
    }
    else {
      Py_RETURN_TRUE;
    }
  }

  JS::RootedValue elementVal(GLOBAL_CX);

  Py_ssize_t index;
  /* Search for the first index where items are different */
  for (index = 0; index < selfLength && index < otherLength; index++) {
    JS_GetElement(GLOBAL_CX, *(self->jsArray), index, &elementVal);

    PyObject *leftItem = pyTypeFactory(GLOBAL_CX, elementVal);
    PyObject *rightItem;

    bool needToDecRefRightItem;
    if (PyObject_TypeCheck(other, &JSArrayProxyType)) {
      JS_GetElement(GLOBAL_CX, *(((JSArrayProxy *)other)->jsArray), index, &elementVal);
      rightItem = pyTypeFactory(GLOBAL_CX, elementVal);
      needToDecRefRightItem = true;
    } else {
      rightItem = ((PyListObject *)other)->ob_item[index];
      needToDecRefRightItem = false;
    }

    if (leftItem == rightItem) {
      continue;
    }

    Py_INCREF(leftItem);
    Py_INCREF(rightItem);
    int k = PyObject_RichCompareBool(leftItem, rightItem, Py_EQ);
    Py_DECREF(leftItem);
    Py_DECREF(rightItem);
    if (k < 0) {
      return NULL;
    }
    if (!k) {
      break;
    }

    Py_DECREF(leftItem);
    if (needToDecRefRightItem) {
      Py_DECREF(rightItem);
    }
  }

  if (index >= selfLength || index >= otherLength) {
    /* No more items to compare -- compare sizes */
    Py_RETURN_RICHCOMPARE(selfLength, otherLength, op);
  }

  /* We have an item that differs -- shortcuts for EQ/NE */
  if (op == Py_EQ) {
    Py_RETURN_FALSE;
  }
  else if (op == Py_NE) {
    Py_RETURN_TRUE;
  }

  JS_GetElement(GLOBAL_CX, *(self->jsArray), index, &elementVal);
  /* Compare the final item again using the proper operator */
  PyObject *pyElementVal = pyTypeFactory(GLOBAL_CX, elementVal);
  PyObject *result = PyObject_RichCompare(pyElementVal, ((PyListObject *)other)->ob_item[index], op);
  Py_DECREF(pyElementVal);
  return result;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_repr(JSArrayProxy *self) {
  Py_ssize_t selfLength = JSArrayProxy_length(self);

  if (selfLength == 0) {
    return PyUnicode_FromString("[]");
  }

  Py_ssize_t i = Py_ReprEnter((PyObject *)self);
  if (i != 0) {
    return i > 0 ? PyUnicode_FromString("[...]") : NULL;
  }

  _PyUnicodeWriter writer;

  _PyUnicodeWriter_Init(&writer);
  writer.overallocate = 1;
  /* "[" + "1" + ", 2" * (len - 1) + "]" */
  writer.min_length = 1 + 1 + (2 + 1) * (selfLength - 1) + 1;

  JS::RootedValue elementVal(GLOBAL_CX);

  if (_PyUnicodeWriter_WriteChar(&writer, '[') < 0) {
    goto error;
  }

  /* Do repr() on each element.  Note that this may mutate the list, so must refetch the list size on each iteration. */
  for (Py_ssize_t index = 0; index < JSArrayProxy_length(self); index++) {
    if (index > 0) {
      if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0) {
        goto error;
      }
    }

    JS_GetElement(GLOBAL_CX, *(self->jsArray), index, &elementVal);

    PyObject *s;
    if (&elementVal.toObject() == (*(self->jsArray)).get()) {
      s = PyObject_Repr((PyObject *)self);
    } else {
      PyObject *pyElementVal = pyTypeFactory(GLOBAL_CX, elementVal);
      s = PyObject_Repr(pyElementVal);
      Py_DECREF(pyElementVal);
    }
    if (s == NULL) {
      goto error;
    }

    if (_PyUnicodeWriter_WriteStr(&writer, s) < 0) {
      Py_DECREF(s);
      goto error;
    }
    Py_DECREF(s);
  }

  writer.overallocate = 0;
  if (_PyUnicodeWriter_WriteChar(&writer, ']') < 0) {
    goto error;
  }

  Py_ReprLeave((PyObject *)self);
  return _PyUnicodeWriter_Finish(&writer);

error:
  _PyUnicodeWriter_Dealloc(&writer);
  Py_ReprLeave((PyObject *)self);
  return NULL;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_iter(JSArrayProxy *self) {
  JSArrayIterProxy *iterator = PyObject_GC_New(JSArrayIterProxy, &JSArrayIterProxyType);
  if (iterator == NULL) {
    return NULL;
  }
  iterator->it.reversed = false;
  iterator->it.it_index = 0;
  Py_INCREF(self);
  iterator->it.it_seq = (PyListObject *)self;
  PyObject_GC_Track(iterator);
  return (PyObject *)iterator;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_iter_reverse(JSArrayProxy *self) {
  JSArrayIterProxy *iterator = PyObject_GC_New(JSArrayIterProxy, &JSArrayIterProxyType);
  if (iterator == NULL) {
    return NULL;
  }
  iterator->it.reversed = true;
  iterator->it.it_index = JSArrayProxyMethodDefinitions::JSArrayProxy_length(self) - 1;
  Py_INCREF(self);
  iterator->it.it_seq = (PyListObject *)self;
  PyObject_GC_Track(iterator);
  return (PyObject *)iterator;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_concat(JSArrayProxy *self, PyObject *value) {
  // value must be a list
  if (!PyList_Check(value)) {
    PyErr_Format(PyExc_TypeError, "can only concatenate list (not \"%.200s\") to list", Py_TYPE(value)->tp_name);
    return NULL;
  }

  Py_ssize_t sizeSelf = JSArrayProxy_length(self);
  Py_ssize_t sizeValue;
  if (PyObject_TypeCheck(value, &JSArrayProxyType)) {
    sizeValue = JSArrayProxyMethodDefinitions::JSArrayProxy_length((JSArrayProxy *)value);
  } else {
    sizeValue = Py_SIZE(value);
  }

  assert((size_t)sizeSelf + (size_t)sizeValue < PY_SSIZE_T_MAX);

  if (sizeValue == 0) {
    if (sizeSelf == 0) {
      return PyList_New(0);
    }
    else {
      Py_INCREF(self);
      return (PyObject *)self;
    }
  }

  JS::RootedObject jCombinedArray(GLOBAL_CX, JS::NewArrayObject(GLOBAL_CX, (size_t)sizeSelf + (size_t)sizeValue));

  JS::RootedValue elementVal(GLOBAL_CX);

  for (Py_ssize_t inputIdx = 0; inputIdx < sizeSelf; inputIdx++) {
    JS_GetElement(GLOBAL_CX, *(self->jsArray), inputIdx, &elementVal);
    JS_SetElement(GLOBAL_CX, jCombinedArray, inputIdx, elementVal);
  }

  if (PyObject_TypeCheck(value, &JSArrayProxyType)) {
    for (Py_ssize_t inputIdx = 0; inputIdx < sizeValue; inputIdx++) {
      JS_GetElement(GLOBAL_CX, *(((JSArrayProxy *)value)->jsArray), inputIdx, &elementVal);
      JS_SetElement(GLOBAL_CX, jCombinedArray, sizeSelf + inputIdx, elementVal);
    }
  } else {
    for (Py_ssize_t inputIdx = 0; inputIdx < sizeValue; inputIdx++) {
      PyObject *item = PyList_GetItem(value, inputIdx);
      elementVal.set(jsTypeFactory(GLOBAL_CX, item));
      JS_SetElement(GLOBAL_CX, jCombinedArray, sizeSelf + inputIdx, elementVal);
    }
  }

  JS::RootedValue jCombinedArrayValue(GLOBAL_CX);
  jCombinedArrayValue.setObjectOrNull(jCombinedArray);

  return pyTypeFactory(GLOBAL_CX, jCombinedArrayValue);
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_repeat(JSArrayProxy *self, Py_ssize_t n) {
  const Py_ssize_t input_size = JSArrayProxy_length(self);
  if (input_size == 0 || n <= 0) {
    return PyList_New(0);
  }

  if (input_size > PY_SSIZE_T_MAX / n) {
    return PyErr_NoMemory();
  }

  JS::RootedObject jCombinedArray(GLOBAL_CX, JS::NewArrayObject(GLOBAL_CX, input_size * n));
  // repeat within new array
  // one might think of using copyWithin but in SpiderMonkey it's implemented in JS!
  JS::RootedValue elementVal(GLOBAL_CX);
  for (Py_ssize_t inputIdx = 0; inputIdx < input_size; inputIdx++) {
    JS_GetElement(GLOBAL_CX, *(self->jsArray), inputIdx, &elementVal);
    for (Py_ssize_t repeatIdx = 0; repeatIdx < n; repeatIdx++) {
      JS_SetElement(GLOBAL_CX, jCombinedArray, repeatIdx * input_size + inputIdx, elementVal);
    }
  }

  JS::RootedValue jCombinedArrayValue(GLOBAL_CX);
  jCombinedArrayValue.setObjectOrNull(jCombinedArray);

  return pyTypeFactory(GLOBAL_CX, jCombinedArrayValue);
}

int JSArrayProxyMethodDefinitions::JSArrayProxy_contains(JSArrayProxy *self, PyObject *element) {
  Py_ssize_t index;
  int cmp;

  Py_ssize_t numElements = JSArrayProxy_length(self);

  JS::RootedValue elementVal(GLOBAL_CX);
  for (index = 0, cmp = 0; cmp == 0 && index < numElements; ++index) {
    JS_GetElement(GLOBAL_CX, *(self->jsArray), index, &elementVal);
    PyObject *item = pyTypeFactory(GLOBAL_CX, elementVal);
    Py_INCREF(item);
    cmp = PyObject_RichCompareBool(item, element, Py_EQ);
    Py_DECREF(item);
    Py_DECREF(item);
  }
  return cmp;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_inplace_concat(JSArrayProxy *self, PyObject *value) {
  Py_ssize_t selfLength = JSArrayProxy_length(self);
  Py_ssize_t valueLength = Py_SIZE(value);

  // allocate extra spacePy_SIZE
  JS::SetArrayLength(GLOBAL_CX, *(self->jsArray), selfLength + valueLength);

  JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, value));
  JS::RootedObject jRootedValue = JS::RootedObject(GLOBAL_CX, jValue.toObjectOrNull());

  JS::RootedValue elementVal(GLOBAL_CX);
  for (Py_ssize_t inputIdx = 0; inputIdx < valueLength; inputIdx++) {
    JS_GetElement(GLOBAL_CX, jRootedValue, inputIdx, &elementVal);
    JS_SetElement(GLOBAL_CX, *(self->jsArray), selfLength + inputIdx, elementVal);
  }

  Py_INCREF(self);
  return (PyObject *)self;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_inplace_repeat(JSArrayProxy *self, Py_ssize_t n) {
  Py_ssize_t input_size = JSArrayProxy_length(self);
  if (input_size == 0 || n == 1) {
    Py_INCREF(self);
    return (PyObject *)self;
  }

  if (n < 1) {
    JSArrayProxy_clear_method(self);
    Py_INCREF(self);
    return (PyObject *)self;
  }

  if (input_size > PY_SSIZE_T_MAX / n) {
    return PyErr_NoMemory();
  }

  JS::SetArrayLength(GLOBAL_CX, *(self->jsArray), input_size * n);

  // repeat within self
  // one might think of using copyWithin but in SpiderMonkey it's implemented in JS!
  JS::RootedValue elementVal(GLOBAL_CX);
  for (Py_ssize_t inputIdx = 0; inputIdx < input_size; inputIdx++) {
    JS_GetElement(GLOBAL_CX, *(self->jsArray), inputIdx, &elementVal);
    for (Py_ssize_t repeatIdx = 0; repeatIdx < n; repeatIdx++) {
      JS_SetElement(GLOBAL_CX, *(self->jsArray), repeatIdx * input_size + inputIdx, elementVal);
    }
  }

  Py_INCREF(self);
  return (PyObject *)self;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_clear_method(JSArrayProxy *self) {
  JS::SetArrayLength(GLOBAL_CX, *(self->jsArray), 0);
  Py_RETURN_NONE;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_copy(JSArrayProxy *self) {
  JS::Rooted<JS::ValueArray<2>> jArgs(GLOBAL_CX);
  jArgs[0].setInt32(0);
  jArgs[1].setInt32(JSArrayProxy_length(self));
  JS::RootedValue jReturnedArray(GLOBAL_CX);
  if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "slice", jArgs, &jReturnedArray)) {
    PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
    return NULL;
  }
  return pyTypeFactory(GLOBAL_CX, jReturnedArray);
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_append(JSArrayProxy *self, PyObject *value) {
  Py_ssize_t len = JSArrayProxy_length(self);

  JS::SetArrayLength(GLOBAL_CX, *(self->jsArray), len + 1);
  JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, value));
  JS_SetElement(GLOBAL_CX, *(self->jsArray), len, jValue);

  Py_RETURN_NONE;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_insert(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs) {
  PyObject *return_value = NULL;
  Py_ssize_t index;
  PyObject *value;

  if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
    return NULL;
  }

  {
    Py_ssize_t ival = -1;
    PyObject *iobj = PyNumber_Index(args[0]);
    if (iobj != NULL) {
      ival = PyLong_AsSsize_t(iobj);
      Py_DECREF(iobj);
    }
    if (ival == -1 && PyErr_Occurred()) {
      return NULL;
    }
    index = ival;
  }

  value = args[1];

  Py_ssize_t n = JSArrayProxy_length(self);

  if (index < 0) {
    index += n;
    if (index < 0) {
      index = 0;
    }
  }
  if (index > n) {
    index = n;
  }

  JS::Rooted<JS::ValueArray<3>> jArgs(GLOBAL_CX);
  jArgs[0].setInt32(index);
  jArgs[1].setInt32(0);
  jArgs[2].set(jsTypeFactory(GLOBAL_CX, value));

  JS::RootedValue jReturnedArray(GLOBAL_CX);
  if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "splice", jArgs, &jReturnedArray)) {
    PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
    return NULL;
  }
  Py_RETURN_NONE;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_extend(JSArrayProxy *self, PyObject *iterable) {
  if (PyList_CheckExact(iterable) || PyTuple_CheckExact(iterable) || (PyObject *)self == iterable) {
    iterable = PySequence_Fast(iterable, "argument must be iterable");
    if (!iterable) {
      return NULL;
    }

    Py_ssize_t n = PySequence_Fast_GET_SIZE(iterable);
    if (n == 0) {
      /* short circuit when iterable is empty */
      Py_DECREF(iterable);
      Py_RETURN_NONE;
    }

    Py_ssize_t m = JSArrayProxy_length(self);

    JS::SetArrayLength(GLOBAL_CX, *(self->jsArray), m + n);

    // populate the end of self with iterable's items.
    PyObject **src = PySequence_Fast_ITEMS(iterable);
    for (Py_ssize_t i = 0; i < n; i++) {
      PyObject *o = src[i];
      JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, o));
      JS_SetElement(GLOBAL_CX, *(self->jsArray), m + i, jValue);
    }

    Py_DECREF(iterable);
  }
  else {
    PyObject *it = PyObject_GetIter(iterable);
    if (it == NULL) {
      return NULL;
    }
    PyObject *(*iternext)(PyObject *) = *Py_TYPE(it)->tp_iternext;

    Py_ssize_t len = JSArrayProxy_length(self);

    for (;; ) {
      PyObject *item = iternext(it);
      if (item == NULL) {
        if (PyErr_Occurred()) {
          if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
            PyErr_Clear();
          }
          else {
            Py_DECREF(it);
            return NULL;
          }
        }
        break;
      }

      JS::SetArrayLength(GLOBAL_CX, *(self->jsArray), len + 1);
      JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, item));
      JS_SetElement(GLOBAL_CX, *(self->jsArray), len, jValue);
      len++;
    }

    Py_DECREF(it);
  }
  Py_RETURN_NONE;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_pop(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs) {
  Py_ssize_t index = -1;

  if (!_PyArg_CheckPositional("pop", nargs, 0, 1)) {
    return NULL;
  }

  if (nargs >= 1) {
    Py_ssize_t ival = -1;
    PyObject *iobj = PyNumber_Index(args[0]);
    if (iobj != NULL) {
      ival = PyLong_AsSsize_t(iobj);
      Py_DECREF(iobj);
    }
    if (ival == -1 && PyErr_Occurred()) {
      return NULL;
    }
    index = ival;
  }

  Py_ssize_t selfSize = JSArrayProxy_length(self);

  if (selfSize == 0) {
    /* Special-case most common failure cause */
    PyErr_SetString(PyExc_IndexError, "pop from empty list");
    return NULL;
  }

  if (index < 0) {
    index += selfSize;
  }

  if ((size_t)index >= (size_t)selfSize) {
    PyErr_SetString(PyExc_IndexError, "pop index out of range");
    return NULL;
  }

  JS::Rooted<JS::ValueArray<2>> jArgs(GLOBAL_CX);
  jArgs[0].setInt32(index);
  jArgs[1].setInt32(1);

  JS::RootedValue jReturnedArray(GLOBAL_CX);
  if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "splice", jArgs, &jReturnedArray)) {
    PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
    return NULL;
  }

  // need the value in the returned array, not the array itself
  JS::RootedObject rootedReturnedArray(GLOBAL_CX, jReturnedArray.toObjectOrNull());
  JS::RootedValue elementVal(GLOBAL_CX);
  JS_GetElement(GLOBAL_CX, rootedReturnedArray, 0, &elementVal);

  return pyTypeFactory(GLOBAL_CX, elementVal);
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_remove(JSArrayProxy *self, PyObject *value) {
  Py_ssize_t selfSize = JSArrayProxy_length(self);

  JS::RootedValue elementVal(GLOBAL_CX);
  for (Py_ssize_t index = 0; index < selfSize; index++) {
    JS_GetElement(GLOBAL_CX, *(self->jsArray), index, &elementVal);
    PyObject *obj = pyTypeFactory(GLOBAL_CX, elementVal);
    Py_INCREF(obj);
    int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
    Py_DECREF(obj);
    Py_DECREF(obj);
    if (cmp > 0) {
      JS::Rooted<JS::ValueArray<2>> jArgs(GLOBAL_CX);
      jArgs[0].setInt32(index);
      jArgs[1].setInt32(1);
      JS::RootedValue jReturnedArray(GLOBAL_CX);
      if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "splice", jArgs, &jReturnedArray)) {
        PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
        return NULL;
      }
      Py_RETURN_NONE;
    }
    else if (cmp < 0) {
      return NULL;
    }
  }

  PyErr_SetString(PyExc_ValueError, "list.remove(x): x not in list");
  return NULL;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_index(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs) {
  PyObject *value;
  Py_ssize_t start = 0;
  Py_ssize_t stop = PY_SSIZE_T_MAX;

  if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
    return NULL;
  }
  value = args[0];
  if (nargs < 2) {
    goto skip_optional;
  }
  if (!_PyEval_SliceIndexNotNone(args[1], &start)) {
    return NULL;
  }
  if (nargs < 3) {
    goto skip_optional;
  }
  if (!_PyEval_SliceIndexNotNone(args[2], &stop)) {
    return NULL;
  }

skip_optional:
  Py_ssize_t selfSize = JSArrayProxy_length(self);

  if (start < 0) {
    start += selfSize;
    if (start < 0) {
      start = 0;
    }
  }
  if (stop < 0) {
    stop += selfSize;
    if (stop < 0) {
      stop = 0;
    }
  }

  JS::RootedValue elementVal(GLOBAL_CX);
  for (Py_ssize_t index = start; index < stop && index < selfSize; index++) {
    JS_GetElement(GLOBAL_CX, *(self->jsArray), index, &elementVal);
    PyObject *obj = pyTypeFactory(GLOBAL_CX, elementVal);
    Py_INCREF(obj);
    int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
    Py_DECREF(obj);
    Py_DECREF(obj);
    if (cmp > 0) {
      return PyLong_FromSsize_t(index);
    }
    else if (cmp < 0) {
      return NULL;
    }
  }

  PyErr_Format(PyExc_ValueError, "%R is not in list", value);
  return NULL;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_count(JSArrayProxy *self, PyObject *value) {
  Py_ssize_t count = 0;

  Py_ssize_t length = JSArrayProxy_length(self);
  JS::RootedValue elementVal(GLOBAL_CX);
  for (Py_ssize_t index = 0; index < length; index++) {
    JS_GetElement(GLOBAL_CX, *(self->jsArray), index, &elementVal);
    PyObject *obj = pyTypeFactory(GLOBAL_CX, elementVal);
    Py_INCREF(obj);
    int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
    Py_DECREF(obj);
    Py_DECREF(obj);
    if (cmp > 0) {
      count++;
    }
    else if (cmp < 0) {
      return NULL;
    }
  }
  return PyLong_FromSsize_t(count);
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_reverse(JSArrayProxy *self) {
  if (JSArrayProxy_length(self) > 1) {
    JS::RootedValue jReturnedArray(GLOBAL_CX);
    if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "reverse", JS::HandleValueArray::empty(), &jReturnedArray)) {
      PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
      return NULL;
    }
  }

  Py_RETURN_NONE;
}

// private
static bool sort_compare_key_func(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject callee(cx, &args.callee());

  JS::RootedValue keyFunc(cx);
  if (!JS_GetProperty(cx, callee, "_key_func_param", &keyFunc)) {
    PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
    return false;
  }
  PyObject *keyfunc = (PyObject *)keyFunc.toPrivate();

  JS::RootedValue reverseValue(cx);
  if (!JS_GetProperty(cx, callee, "_reverse_param", &reverseValue)) {
    PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
    return false;
  }
  bool reverse = reverseValue.toBoolean();

  JS::RootedValue elementVal0(cx, args[0]);
  PyObject *args_0 = pyTypeFactory(cx, elementVal0);
  PyObject *args_0_result = PyObject_CallFunction(keyfunc, "O", args_0);
  Py_DECREF(args_0);
  if (!args_0_result) {
    return false;
  }

  JS::RootedValue elementVal1(cx, args[1]);
  PyObject *args_1 = pyTypeFactory(cx, elementVal1);
  PyObject *args_1_result = PyObject_CallFunction(keyfunc, "O", args_1);
  Py_DECREF(args_1);
  if (!args_1_result) {
    return false;
  }

  int cmp = PyObject_RichCompareBool(args_0_result, args_1_result, Py_LT);
  if (cmp > 0) {
    args.rval().setInt32(reverse ? 1 : -1);
  } else if (cmp == 0) {
    cmp = PyObject_RichCompareBool(args_0_result, args_1_result, Py_EQ);
    if (cmp > 0) {
      args.rval().setInt32(0);
    }
    else if (cmp == 0) {
      args.rval().setInt32(reverse ? -1 : 1);
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }

  return true;
}

// private
static bool sort_compare_default(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject callee(cx, &args.callee());
  JS::RootedValue reverseValue(cx);
  if (!JS_GetProperty(cx, callee, "_reverse_param", &reverseValue)) {
    PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
    return false;
  }
  bool reverse = reverseValue.toBoolean();

  JS::RootedValue elementVal0(cx, args[0]);
  PyObject *args_0 = pyTypeFactory(cx, elementVal0);

  JS::RootedValue elementVal1(cx, args[1]);
  PyObject *args_1 = pyTypeFactory(cx, elementVal1);

  int cmp = PyObject_RichCompareBool(args_0, args_1, Py_LT);
  if (cmp > 0) {
    args.rval().setInt32(reverse ? 1 : -1);
  }
  else if (cmp == 0) {
    cmp = PyObject_RichCompareBool(args_0, args_1, Py_EQ);
    if (cmp > 0) {
      args.rval().setInt32(0);
    }
    else if (cmp == 0) {
      args.rval().setInt32(reverse ? -1 : 1);
    }
    else {
      Py_DECREF(args_0);
      Py_DECREF(args_1);
      return false;
    }
  }
  else {
    Py_DECREF(args_0);
    Py_DECREF(args_1);
    return false;
  }

  Py_DECREF(args_0);
  Py_DECREF(args_1);
  return true;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_sort(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
  #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

  #define NUM_KEYWORDS 2
  static struct {
    PyGC_Head _this_is_not_used;
    PyObject_VAR_HEAD
    PyObject *ob_item[NUM_KEYWORDS];
  } _kwtuple = {
    .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
    .ob_item = {&_Py_ID(key), &_Py_ID(reverse), },
  };
  #undef NUM_KEYWORDS
  #define KWTUPLE (&_kwtuple.ob_base.ob_base)

  #else // !Py_BUILD_CORE
  #  define KWTUPLE NULL
  #endif // !Py_BUILD_CORE

  static const char *const _keywords[] = {"key", "reverse", NULL};
  static _PyArg_Parser _parser = {
    .keywords = _keywords,
    .fname = "sort",
    .kwtuple = KWTUPLE,
  };
  #undef KWTUPLE

  PyObject *argsbuf[2];
  Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
  PyObject *keyfunc = Py_None;
  int reverse = 0;

  args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, argsbuf);
  if (!args) {
    return NULL;
  }

  if (!noptargs) {
    goto skip_optional_kwonly;
  }

  if (args[0]) {
    keyfunc = args[0];
    if (!--noptargs) {
      goto skip_optional_kwonly;
    }
  }

  reverse = PyObject_IsTrue(args[1]);
  if (reverse < 0) {
    return NULL;
  }

skip_optional_kwonly:
  if (JSArrayProxy_length(self) > 1) {
    JS::RootedValue jReturnedArray(GLOBAL_CX);
    if (keyfunc != Py_None) {
      if (PyFunction_Check(keyfunc)) {
        // we got a python key function, check if two-argument js style or standard python 1-arg
        PyObject *code = PyFunction_GetCode(keyfunc);
        if (((PyCodeObject *)code)->co_argcount == 1) {
          // adapt to python style, provide js-style comp wrapper that does it the python way, which is < based with calls to keyFunc
          JS::RootedObject funObj(GLOBAL_CX, JS_GetFunctionObject(JS_NewFunction(GLOBAL_CX, sort_compare_key_func, 2, 0, NULL)));

          JS::RootedValue privateValue(GLOBAL_CX, JS::PrivateValue(keyfunc));
          if (!JS_SetProperty(GLOBAL_CX, funObj, "_key_func_param", privateValue)) {  // JS::SetReservedSlot(functionObj, KeyFuncSlot, JS::PrivateValue(keyfunc)); does not work
            PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
            return NULL;
          }

          JS::RootedValue reverseValue(GLOBAL_CX);
          reverseValue.setBoolean(reverse);
          if (!JS_SetProperty(GLOBAL_CX, funObj, "_reverse_param", reverseValue)) {
            PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
            return NULL;
          }

          JS::Rooted<JS::ValueArray<1>> jArgs(GLOBAL_CX);
          jArgs[0].setObject(*funObj);
          if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "sort", jArgs, &jReturnedArray)) {
            if (!PyErr_Occurred()) {
              PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
            }
            return NULL;
          }

          // cleanup
          if (!JS_DeleteProperty(GLOBAL_CX, funObj, "_key_func_param")) {
            PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
            return NULL;
          }

          if (!JS_DeleteProperty(GLOBAL_CX, funObj, "_reverse_param")) {
            PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
            return NULL;
          }
        }
        else {
          // two-arg js-style
          JS::Rooted<JS::ValueArray<1>> jArgs(GLOBAL_CX);
          jArgs[0].set(jsTypeFactory(GLOBAL_CX, keyfunc));
          if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "sort", jArgs, &jReturnedArray)) {
            PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
            return NULL;
          }

          if (reverse) {
            JSArrayProxy_reverse(self);
          }
        }
      }
      else if (PyObject_TypeCheck(keyfunc, &JSFunctionProxyType)) {
        JS::Rooted<JS::ValueArray<1>> jArgs(GLOBAL_CX);
        jArgs[0].setObject(**((JSFunctionProxy *)keyfunc)->jsFunc);
        if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "sort", jArgs, &jReturnedArray)) {
          PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
          return NULL;
        }

        if (reverse) {
          JSArrayProxy_reverse(self);
        }
      }
      else if (PyCFunction_Check(keyfunc)) {
        JS::RootedObject funObj(GLOBAL_CX, JS_GetFunctionObject(JS_NewFunction(GLOBAL_CX, sort_compare_key_func, 2, 0, NULL)));

        JS::RootedValue privateValue(GLOBAL_CX, JS::PrivateValue(keyfunc));
        if (!JS_SetProperty(GLOBAL_CX, funObj, "_key_func_param", privateValue)) {  // JS::SetReservedSlot(functionObj, KeyFuncSlot, JS::PrivateValue(keyfunc)); does not work
          PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
          return NULL;
        }

        JS::RootedValue reverseValue(GLOBAL_CX);
        reverseValue.setBoolean(reverse);
        if (!JS_SetProperty(GLOBAL_CX, funObj, "_reverse_param", reverseValue)) {
          PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
          return NULL;
        }

        JS::Rooted<JS::ValueArray<1>> jArgs(GLOBAL_CX);
        jArgs[0].setObject(*funObj);
        if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "sort", jArgs, &jReturnedArray)) {
          if (!PyErr_Occurred()) {
            PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
          }
          return NULL;
        }
      }
      else {
        PyErr_Format(PyExc_TypeError, "'%.200s' object is not callable", Py_TYPE(keyfunc)->tp_name);
        return NULL;
      }
    }
    else {
      // adapt to python style, provide js-style comp wrapper that does it the python way, which is < based
      JSFunction *cmpFunction = JS_NewFunction(GLOBAL_CX, sort_compare_default, 2, 0, NULL);
      JS::RootedObject funObj(GLOBAL_CX, JS_GetFunctionObject(cmpFunction));

      JS::RootedValue reverseValue(GLOBAL_CX);
      reverseValue.setBoolean(reverse);
      if (!JS_SetProperty(GLOBAL_CX, funObj, "_reverse_param", reverseValue)) {
        PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
        return NULL;
      }

      JS::Rooted<JS::ValueArray<1>> jArgs(GLOBAL_CX);
      jArgs[0].setObject(*funObj);
      if (!JS_CallFunctionName(GLOBAL_CX, *(self->jsArray), "sort", jArgs, &jReturnedArray)) {
        PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSArrayProxyType.tp_name);
        return NULL;
      }
    }
  }
  Py_RETURN_NONE;
}