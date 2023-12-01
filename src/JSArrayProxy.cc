/**
 * @file JSArrayProxy.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief JSArrayProxy is a custom C-implemented python type that derives from list. It acts as a proxy for JSArrays from Spidermonkey, and behaves like a list would.
 * @version 0.1
 * @date 2023-11-22
 *
 * Copyright (c) 2023 Distributive Corp.
 *
 */


#include "include/JSArrayProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/PyProxyHandler.hh"

#include <jsapi.h>
#include <jsfriendapi.h>

#include <Python.h>


void JSArrayProxyMethodDefinitions::JSArrayProxy_dealloc(JSArrayProxy *self)
{
  PyObject_GC_UnTrack(self);
  Py_TYPE(self)->tp_free((PyObject *)self);
  self->jsObject.set(nullptr);
  return;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{
  PyObject *self = PyDict_Type.tp_new(subtype, args, kwds);
  ((JSArrayProxy *)self)->jsObject = JS::RootedObject(GLOBAL_CX, nullptr);
  return self;
}

int JSArrayProxyMethodDefinitions::JSArrayProxy_init(JSArrayProxy *self, PyObject *args, PyObject *kwds)
{
  // make fresh JSArray for proxy
  // TODO args??
  self->jsObject.set(JS::NewArrayObject(GLOBAL_CX, 0));
  return 0;
}

Py_ssize_t JSArrayProxyMethodDefinitions::JSArrayProxy_length(JSArrayProxy *self)
{
  uint32_t length;
  JS::GetArrayLength(GLOBAL_CX, self->jsObject, &length);
  return (Py_ssize_t)length;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_get(JSArrayProxy *self, PyObject *key)
{
  JS::RootedId id(GLOBAL_CX);
  if (!keyToId(key, &id)) {
    // TODO (Caleb Aikens): raise exception here
    return NULL; // key is not a str or int
  }

  JS::RootedValue *value = new JS::RootedValue(GLOBAL_CX);
  JS_GetPropertyById(GLOBAL_CX, self->jsObject, id, value);

  JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));

  if (!value->isUndefined()) {
    return pyTypeFactory(GLOBAL_CX, global, value)->getPyObject();
  }
  else {
    // look through the methods for dispatch
    for (size_t index = 0;; index++) {
      const char *methodName = JSArrayProxyType.tp_methods[index].ml_name;
      if (methodName == NULL) { // reached end of list
        return pyTypeFactory(GLOBAL_CX, global, value)->getPyObject();
      }
      else if (PyUnicode_Check(key)) {
        if (strcmp(methodName, PyUnicode_AsUTF8(key)) == 0) {
          return PyObject_GenericGetAttr((PyObject *)self, key);
        }
      }
      else {
        return pyTypeFactory(GLOBAL_CX, global, value)->getPyObject();
      }
    }
  }
}

int JSArrayProxyMethodDefinitions::JSArrayProxy_assign_key(JSArrayProxy *self, PyObject *key, PyObject *value)
{
  JS::RootedId id(GLOBAL_CX);
  if (!keyToId(key, &id)) { // invalid key
    // TODO (Caleb Aikens): raise exception here
    return -1;
  }

  if (value) { // we are setting a value
    JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, value));
    JS_SetPropertyById(GLOBAL_CX, self->jsObject, id, jValue);
  } else { // we are deleting a value
    JS::ObjectOpResult ignoredResult;
    JS_DeletePropertyById(GLOBAL_CX, self->jsObject, id, ignoredResult);
  }

  return 0;
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_richcompare(JSArrayProxy *self, PyObject *other, int op)
{
  if (!PyList_Check(self) || !PyList_Check(other)) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  if (JSArrayProxy_length(self) != Py_SIZE((PyListObject *)other) && (op == Py_EQ || op == Py_NE)) {
    /* Shortcut: if the lengths differ, the lists differ */
    if (op == Py_EQ) {
      Py_RETURN_FALSE;
    }
    else {
      Py_RETURN_TRUE;
    }
  }

  Py_ssize_t selfLength = JSArrayProxy_length(self);
  Py_ssize_t otherLength = Py_SIZE(other);

  JS::RootedValue elementVal(GLOBAL_CX);
  JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));

  Py_ssize_t index;
  /* Search for the first index where items are different */
  for (index = 0; index < selfLength && index < otherLength; index++) {
    JS_GetElement(GLOBAL_CX, self->jsObject, index, &elementVal);

    PyObject *leftItem = pyTypeFactory(GLOBAL_CX, global, &elementVal)->getPyObject();
    PyObject *rightItem = ((PyListObject *)other)->ob_item[index];
    if (leftItem == rightItem) {
      continue;
    }

    Py_INCREF(leftItem);
    Py_INCREF(rightItem);
    int k = PyObject_RichCompareBool(leftItem, rightItem, Py_EQ);
    Py_DECREF(leftItem);
    Py_DECREF(rightItem);
    if (k < 0)
      return NULL;
    if (!k)
      break;
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

  JS_GetElement(GLOBAL_CX, self->jsObject, index, &elementVal);
  /* Compare the final item again using the proper operator */
  return PyObject_RichCompare(pyTypeFactory(GLOBAL_CX, global, &elementVal)->getPyObject(), ((PyListObject *)other)->ob_item[index], op);
}

PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_repr(JSArrayProxy *self) {
  if (JSArrayProxy_length(self) == 0) {
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
  writer.min_length = 1 + 1 + (2 + 1) * (JSArrayProxy_length(self) - 1) + 1;

  JS::RootedValue *elementVal = new JS::RootedValue(GLOBAL_CX);
  JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));

  if (_PyUnicodeWriter_WriteChar(&writer, '[') < 0) {
    goto error;
  }

  for (Py_ssize_t index = 0; index < JSArrayProxy_length(self); ++index) {
    if (index > 0) {
      if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0) {
        goto error;
      }
    }

    JS_GetElement(GLOBAL_CX, self->jsObject, index, elementVal);

    PyObject *s = PyObject_Repr(pyTypeFactory(GLOBAL_CX, global, elementVal)->getPyObject());
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
  JSContext *cx = GLOBAL_CX;
  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(self->jsObject));

  // Get **enumerable** own properties
  JS::RootedIdVector props(cx);
  if (!js::GetPropertyKeys(cx, self->jsObject, JSITER_OWNONLY, &props)) {
    return NULL;
  }

  // Populate a Python tuple with (propertyKey, value) pairs from the JS object
  // Similar to `Object.entries()`
  size_t length = props.length();
  PyObject *seq = PyTuple_New(length);
  for (size_t i = 0; i < length; i++) {
    JS::HandleId id = props[i];
    PyObject *key = idToKey(cx, id);

    JS::RootedValue *jsVal = new JS::RootedValue(cx);
    JS_GetPropertyById(cx, self->jsObject, id, jsVal);
    PyObject *value = pyTypeFactory(cx, global, jsVal)->getPyObject();

    PyTuple_SetItem(seq, i, PyTuple_Pack(2, key, value));
  }

  // Convert to a Python iterator
  return PyObject_GetIter(seq);
}

/*
   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_concat(JSArrayProxy *self, PyObject *value) {
   printf("JSArrayProxy_concat\n");

   // value must be a list
   if (!PyList_Check(value)) {
    PyErr_Format(PyExc_TypeError, "can only concatenate list (not \"%.200s\") to list", Py_TYPE(value)->tp_name);
    return NULL;
   }

   assert((size_t)JSArrayProxy_length(self) + (size_t)Py_SIZE(value) < PY_SSIZE_T_MAX);

   Py_ssize_t size = JSArrayProxy_length(self) + Py_SIZE(value);
   if (size == 0) {
    return PyList_New(0);
   }

   JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, value));
   // jValue is a JSArray since value is a List
   JS::Rooted<JS::ValueArray<1>> args(GLOBAL_CX);
   args[0].setObject(jValue.toObject());
   JS::RootedValue *jCombinedArray = new JS::RootedValue(GLOBAL_CX);
   JS_CallFunctionName(GLOBAL_CX, self->jsObject, "concat", args, jCombinedArray);

   JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
   return pyTypeFactory(GLOBAL_CX, global, jCombinedArray)->getPyObject();
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_repeat(JSArrayProxy *self, Py_ssize_t n) {
   printf("JSArrayProxy_repeat\n");

   const Py_ssize_t input_size = JSArrayProxy_length(self);
   if (input_size == 0 || n <= 0) {
    return PyList_New(0);
   }
   assert(n > 0);

   if (input_size > PY_SSIZE_T_MAX / n) {
    return PyErr_NoMemory();
   }

   JS::RootedObject jCombinedArray = JS::RootedObject(GLOBAL_CX, JS::NewArrayObject(GLOBAL_CX, input_size * n));
   // repeat within new array
   // one might think of using copyWithin but in SpiderMonkey it's implemented in JS!
   JS::RootedValue elementVal(GLOBAL_CX);
   for (Py_ssize_t inputIdx = 0; inputIdx < input_size; inputIdx++) {
    JS_GetElement(GLOBAL_CX, self->jsObject, inputIdx, &elementVal);
    for (Py_ssize_t repeatIdx = 0; repeatIdx < n; repeatIdx++) {
      JS_SetElement(GLOBAL_CX, jCombinedArray, repeatIdx * input_size + inputIdx, elementVal);
    }
   }

   JS::RootedValue *jCombinedArrayValue = new JS::RootedValue(GLOBAL_CX);
   jCombinedArrayValue->setObjectOrNull(jCombinedArray);

   JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
   return pyTypeFactory(GLOBAL_CX, global, jCombinedArrayValue)->getPyObject();
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_item(JSArrayProxy *self, Py_ssize_t index)
   {
   printf("JSArrayProxy_item\n");
   if ((size_t)index >= (size_t)JSArrayProxy_length(self)) {
    PyErr_SetObject(PyExc_IndexError, PyUnicode_FromString("list index out of range"));
    return NULL;
   }

   JS::RootedValue *elementVal = new JS::RootedValue(GLOBAL_CX);
   JS_GetElement(GLOBAL_CX, self->jsObject, index, elementVal);

   JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
   return pyTypeFactory(GLOBAL_CX, global, elementVal)->getPyObject();
   }

   int JSArrayProxyMethodDefinitions::JSArrayProxy_assign_index(JSArrayProxy *self, Py_ssize_t index, PyObject *value) {
   printf("JSArrayProxy_assign_index\n");

   if ((size_t)index >= (size_t)JSArrayProxy_length(self)) {
    PyErr_SetObject(PyExc_IndexError, PyUnicode_FromString("list assignment out of range"));
    return -1;
   }

   if (value == NULL) {
    // TODO
   }

   JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, value));
   JS_SetElement(GLOBAL_CX, self->jsObject, index, jValue);
   return 0;
   }

   int JSArrayProxyMethodDefinitions::JSArrayProxy_contains(JSArrayProxy *self, PyObject *element) {
   printf("JSArrayProxy_contains\n");

   Py_ssize_t numElements = JSArrayProxy_length(self);
   JS::RootedObject global(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
   JS::RootedValue elementVal(GLOBAL_CX);
   for (Py_ssize_t index = 0; index < numElements; ++index) {
    JS_GetElement(GLOBAL_CX, self->jsObject, index, &elementVal);
    // THIS looks wrong, we should not be comparing self ???? TODO
    PyObject *isEqual = JSArrayProxyMethodDefinitions::JSArrayProxy_richcompare(self, pyTypeFactory(GLOBAL_CX, &global, &elementVal)->getPyObject(), Py_EQ);
    if (isEqual == NULL) {
      return -1;
    } else if (Py_IsTrue(isEqual)) {
      return 1;
    }
   }
   return 0;
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_inplace_concat(JSArrayProxy *self, PyObject *value) {
   printf("JSArrayProxy_inplace_concat\n");
   // TODO
   return NULL;
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_inplace_repeat(JSArrayProxy *self, Py_ssize_t n) {
   printf("JSArrayProxy_inplace_repeat\n");

   Py_ssize_t input_size = JSArrayProxy_length(self);
   if (input_size == 0 || n == 1) {
    return Py_NewRef(self);
   }

   if (n < 1) {
    JSArrayProxy_clear(self);
    return Py_NewRef(self);
   }

   if (input_size > PY_SSIZE_T_MAX / n) {
    return PyErr_NoMemory();
   }

   JS::SetArrayLength(GLOBAL_CX, self->jsObject, input_size * n);

   // repeat within self
   // one might think of using copyWithin but in SpiderMonkey it's implemented in JS!
   JS::RootedValue elementVal(GLOBAL_CX);
   for (Py_ssize_t inputIdx = 0; inputIdx < input_size; inputIdx++) {
    JS_GetElement(GLOBAL_CX, self->jsObject, inputIdx, &elementVal);
    for (Py_ssize_t repeatIdx = 0; repeatIdx < n; repeatIdx++) {
      JS_SetElement(GLOBAL_CX, self->jsObject, repeatIdx * input_size + inputIdx, elementVal);
    }
   }

   return Py_NewRef(self);
   }

   int JSArrayProxyMethodDefinitions::JSArrayProxy_clear(JSArrayProxy *self) {
   printf("JSArrayProxy_clear\n");
   JS::SetArrayLength(GLOBAL_CX, self->jsObject, 0);
   return 0;
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_copy(JSArrayProxy *self) {
   printf("JSArrayProxy_copy\n");

   JS::Rooted<JS::ValueArray<2>> jArgs(GLOBAL_CX); // TODO needs to be on the stack?
   jArgs[0].setInt32(0);
   jArgs[1].setInt32(JSArrayProxy_length(self));
   JS::RootedValue *jReturnedArray = new JS::RootedValue(GLOBAL_CX);
   JS_CallFunctionName(GLOBAL_CX, self->jsObject, "slice", jArgs, jReturnedArray);

   JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
   return pyTypeFactory(GLOBAL_CX, global, jReturnedArray)->getPyObject();
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_append(JSArrayProxy *self, PyObject *value) {
   printf("JSArrayProxy_append\n");

   assert(self != NULL && value != NULL);
   assert(PyList_Check(self));

   Py_ssize_t len = JSArrayProxy_length(self);

   // PyObject *inserted = Py_NewRef(value);
   JS::SetArrayLength(GLOBAL_CX, self->jsObject, len + 1);
   JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, value));
   JS_SetElement(GLOBAL_CX, self->jsObject, len, jValue);

   Py_RETURN_NONE;
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_insert(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs) {
   printf("JSArrayProxy_insert\n");

   PyObject *return_value = NULL;
   Py_ssize_t index;
   PyObject *object;

   if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
    return NULL;
   }

   {
    Py_ssize_t ival = -1;
    PyObject *iobj = _PyNumber_Index(args[0]);
    if (iobj != NULL) {
      ival = PyLong_AsSsize_t(iobj);
      Py_DECREF(iobj);
    }
    if (ival == -1 && PyErr_Occurred()) {
      return NULL;
    }
    index = ival;
   }

   object = args[1];

   JS::Rooted<JS::ValueArray<3>> jArgs(GLOBAL_CX); // TODO needs to be on the heap?
   jArgs[0].setInt32(index);
   jArgs[1].setInt32(1);
   JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, object));
   jArgs[1].setObject(jValue.toObject());
   JS::RootedValue *jReturnedArray = new JS::RootedValue(GLOBAL_CX);
   JS_CallFunctionName(GLOBAL_CX, self->jsObject, "splice", jArgs, jReturnedArray);

   Py_RETURN_NONE;
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_extend(JSArrayProxy *self, PyObject *iterable) {
   printf("JSArrayProxy_extend\n");

   // Special cases:
   // 1) lists and tuples which can use PySequence_Fast ops
   // 2) extending self to self requires making a copy first
   if (PyList_CheckExact(iterable) || PyTuple_CheckExact(iterable) || (PyObject *)self == iterable) {
    iterable = PySequence_Fast(iterable, "argument must be iterable");
    if (!iterable) {
      return NULL;
    }

    Py_ssize_t n = PySequence_Fast_GET_SIZE(iterable);
    if (n == 0) {
      /* short circuit when iterable is empty *//*
      Py_RETURN_NONE;
    }

    Py_ssize_t m = JSArrayProxy_length(self);
    // It should not be possible to allocate a list large enough to cause
    // an overflow on any relevant platform.
    assert(m < PY_SSIZE_T_MAX - n);
    JS::SetArrayLength(GLOBAL_CX, self->jsObject, m + n);

    // note that we may still have self == iterable here for the
    // situation a.extend(a), but the following code works
    // in that case too.  Just make sure to resize self
    // before calling PySequence_Fast_ITEMS.
    //
    // populate the end of self with iterable's items.
    PyObject **src = PySequence_Fast_ITEMS(iterable);
    for (Py_ssize_t i = 0; i < n; i++) {
      PyObject *o = src[i];
      // dest[i] = Py_NewRef(o); TODO NewRef needed?
      JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, o));
      JS_SetElement(GLOBAL_CX, self->jsObject, m + i, jValue);
    }

    Py_DECREF(iterable);
    Py_RETURN_NONE;
   }
   else {
    PyObject *it = PyObject_GetIter(iterable);
    if (it == NULL) {
      return NULL;
    }
    PyObject *(*iternext)(PyObject *) = *Py_TYPE(it)->tp_iternext;


    Py_ssize_t len = JSArrayProxy_length(self);

    /* Run iterator to exhaustion. *//*
    for (;; ) {
      PyObject *item = iternext(it);
      if (item == NULL) {
        if (PyErr_Occurred()) {
          if (PyErr_ExceptionMatches(PyExc_StopIteration))
            PyErr_Clear();
          else
            goto error;
        }
        break;
      }

      JS::SetArrayLength(GLOBAL_CX, self->jsObject, len + 1);
      JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, item));
      JS_SetElement(GLOBAL_CX, self->jsObject, len, jValue);
      len++;
    }

    Py_DECREF(it);
    Py_RETURN_NONE;

   error:
    Py_DECREF(it);
    return NULL;
   }
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_pop(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs) {
   printf("JSArrayProxy_pop\n");

   PyObject *return_value = NULL;
   Py_ssize_t index = -1;

   if (!_PyArg_CheckPositional("pop", nargs, 0, 1)) {
    return NULL;
   }

   if (nargs >= 1) {
    Py_ssize_t ival = -1;
    PyObject *iobj = _PyNumber_Index(args[0]);
    if (iobj != NULL) {
      ival = PyLong_AsSsize_t(iobj);
      Py_DECREF(iobj);
    }
    if (ival == -1 && PyErr_Occurred()) {
      return return_value;
    }
    index = ival;
   }

   JS::Rooted<JS::ValueArray<2>> jArgs(GLOBAL_CX); // TODO needs to be on the heap?
   jArgs[0].setInt32(index);
   jArgs[1].setInt32(1);
   JS::RootedValue *jReturnedArray = new JS::RootedValue(GLOBAL_CX);
   JS_CallFunctionName(GLOBAL_CX, self->jsObject, "splice", jArgs, jReturnedArray);

   JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
   return pyTypeFactory(GLOBAL_CX, global, jReturnedArray)->getPyObject();
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_remove(JSArrayProxy *self, PyObject *value) {
   printf("JSArrayProxy_remove\n");

   JS::RootedObject global(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
   JS::RootedValue elementVal(GLOBAL_CX);
   for (Py_ssize_t index = 0; index < JSArrayProxy_length(self); index++) {
    JS_GetElement(GLOBAL_CX, self->jsObject, index, &elementVal);
    PyObject *obj = pyTypeFactory(GLOBAL_CX, &global, &elementVal)->getPyObject();
    Py_INCREF(obj);
    //  int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
   // TODO
    Py_DECREF(obj);

    JS::Rooted<JS::ValueArray<2>> jArgs(GLOBAL_CX); // TODO needs to be on the heap?
    jArgs[0].setInt32(index);
    jArgs[1].setInt32(1);
    JS::RootedValue *jReturnedArray = new JS::RootedValue(GLOBAL_CX);
    JS_CallFunctionName(GLOBAL_CX, self->jsObject, "splice", jArgs, jReturnedArray);
    Py_RETURN_NONE;
   }
   PyErr_Format(PyExc_ValueError, "%R is not in list", value);
   return NULL;

   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_index(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs) {
   printf("JSArrayProxy_index\n");

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
   if (start < 0) {
    start += JSArrayProxy_length(self);
    if (start < 0)
      start = 0;
   }
   if (stop < 0) {
    stop += JSArrayProxy_length(self);
    if (stop < 0)
      stop = 0;
   }

   Py_ssize_t length = JSArrayProxy_length(self);
   JS::RootedObject global(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
   JS::RootedValue elementVal(GLOBAL_CX);
   for (Py_ssize_t index = start; index < stop && index < length; index++) {
    JS_GetElement(GLOBAL_CX, self->jsObject, index, &elementVal);
    PyObject *obj = pyTypeFactory(GLOBAL_CX, &global, &elementVal)->getPyObject();
    Py_INCREF(obj);
    // TODO
    int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
    Py_DECREF(obj);
    if (cmp > 0)
      return PyLong_FromSsize_t(index);
    else if (cmp < 0)
      return NULL;
   }
   PyErr_Format(PyExc_ValueError, "%R is not in list", value);
   return NULL;
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_count(JSArrayProxy *self, PyObject *value) {
   printf("JSArrayProxy_count\n");

   Py_ssize_t count = 0;

   Py_ssize_t length = JSArrayProxy_length(self);
   JS::RootedObject global(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
   JS::RootedValue elementVal(GLOBAL_CX);
   for (Py_ssize_t index = 0; index < length; index++) {
    JS_GetElement(GLOBAL_CX, self->jsObject, index, &elementVal);
    PyObject *obj = pyTypeFactory(GLOBAL_CX, &global, &elementVal)->getPyObject();
    // PyObject *obj = self->ob_item[i];
    if (obj == value) {
      count++;
      continue;
    }
    Py_INCREF(obj);
    // TODO
    int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
    Py_DECREF(obj);
    if (cmp > 0)
      count++;
    else if (cmp < 0)
      return NULL;
   }
   return PyLong_FromSsize_t(count);
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_reverse(JSArrayProxy *self) {
   printf("JSArrayProxy_reverse\n");

   if (JSArrayProxy_length(self) > 1) {
    JS::RootedValue *jReturnedArray = new JS::RootedValue(GLOBAL_CX);
    JS_CallFunctionName(GLOBAL_CX, self->jsObject, "reverse", JS::HandleValueArray::empty(), jReturnedArray);
   }

   Py_RETURN_NONE;
   }

   PyObject *JSArrayProxyMethodDefinitions::JSArrayProxy_sort(JSArrayProxy *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
   printf("JSArrayProxy_sort\n");

   PyObject *return_value = NULL;
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
    goto exit;
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
    goto exit;
   }

   skip_optional_kwonly:
   if (JSArrayProxy_length(self) > 1) {
    JS::RootedValue *jReturnedArray = new JS::RootedValue(GLOBAL_CX);
    if (keyfunc != Py_None) {
      JS::Rooted<JS::ValueArray<1>> jArgs(GLOBAL_CX); // TODO needs to be on the heap?
      JS::RootedValue jValue(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, keyfunc));
      jArgs[0].setObject(jValue.toObject());
      JS_CallFunctionName(GLOBAL_CX, self->jsObject, "sort", jArgs, jReturnedArray);
    } else {
      JS::RootedValue *jReturnedArray = new JS::RootedValue(GLOBAL_CX);
      JS_CallFunctionName(GLOBAL_CX, self->jsObject, "sort", JS::HandleValueArray::empty(), jReturnedArray);
    }

    if (reverse) {
      JS_CallFunctionName(GLOBAL_CX, self->jsObject, "reverse", JS::HandleValueArray::empty(), jReturnedArray);
    }
   }

   Py_RETURN_NONE;

   exit:
   return return_value;
   }
 */

