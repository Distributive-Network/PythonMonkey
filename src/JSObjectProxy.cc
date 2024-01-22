/**
 * @file JSObjectProxy.cc
 * @author Caleb Aikens (caleb@distributive.network), Tom Tang (xmader@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief JSObjectProxy is a custom C-implemented python type that derives from dict. It acts as a proxy for JSObjects from Spidermonkey, and behaves like a dict would.
 * @version 0.1
 * @date 2023-06-26
 *
 * Copyright (c) 2023-2024 Distributive Corp.
 *
 */

#include "include/JSObjectProxy.hh"

#include "include/JSObjectIterProxy.hh"

#include "include/JSObjectKeysProxy.hh"
#include "include/JSObjectValuesProxy.hh"
#include "include/JSObjectItemsProxy.hh"

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/PyBaseProxyHandler.hh"

#include <jsapi.h>
#include <jsfriendapi.h>

#include <Python.h>

#include <object.h>

JSContext *GLOBAL_CX; /**< pointer to PythonMonkey's JSContext */

bool keyToId(PyObject *key, JS::MutableHandleId idp) {
  if (PyUnicode_Check(key)) { // key is str type
    JS::RootedString idString(GLOBAL_CX);
    const char *keyStr = PyUnicode_AsUTF8(key);
    JS::ConstUTF8CharsZ utf8Chars(keyStr, strlen(keyStr));
    idString.set(JS_NewStringCopyUTF8Z(GLOBAL_CX, utf8Chars));
    return JS_StringToId(GLOBAL_CX, idString, idp);
  } else if (PyLong_Check(key)) { // key is int type
    uint32_t keyAsInt = PyLong_AsUnsignedLong(key); // raise OverflowError if the value of pylong is out of range for a unsigned long
    return JS_IndexToId(GLOBAL_CX, keyAsInt, idp);
  } else {
    return false; // fail
  }
}

void JSObjectProxyMethodDefinitions::JSObjectProxy_dealloc(JSObjectProxy *self)
{
  // TODO (Caleb Aikens): intentional override of PyDict_Type's tp_dealloc. Probably results in leaking dict memory
  self->jsObject.set(nullptr);
  PyObject_GC_UnTrack(self);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{
  return PyDict_Type.tp_new(subtype, args, kwds);
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_init(JSObjectProxy *self, PyObject *args, PyObject *kwds)
{
  if (PyDict_Type.tp_init((PyObject *)self, args, kwds) < 0) {
    return -1;
  }
  return 0;
}

Py_ssize_t JSObjectProxyMethodDefinitions::JSObjectProxy_length(JSObjectProxy *self)
{
  JS::RootedIdVector props(GLOBAL_CX);
  if (!js::GetPropertyKeys(GLOBAL_CX, self->jsObject, JSITER_OWNONLY | JSITER_HIDDEN, &props))
  {
    // @TODO (Caleb Aikens) raise exception here
    return -1;
  }
  return props.length();
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_get(JSObjectProxy *self, PyObject *key)
{
  JS::RootedId id(GLOBAL_CX);
  if (!keyToId(key, &id)) {
    // TODO (Caleb Aikens): raise exception here
    return NULL; // key is not a str or int
  }

  // look through the methods for dispatch
  for (size_t index = 0;; index++) {
    const char *methodName = JSObjectProxyType.tp_methods[index].ml_name;
    if (methodName == NULL) { // reached end of list
      JS::RootedValue *value = new JS::RootedValue(GLOBAL_CX);
      JS_GetPropertyById(GLOBAL_CX, self->jsObject, id, value);
      JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
      return pyTypeFactory(GLOBAL_CX, global, value)->getPyObject();
    }
    else if (PyUnicode_Check(key)) {
      if (strcmp(methodName, PyUnicode_AsUTF8(key)) == 0) {
        return PyObject_GenericGetAttr((PyObject *)self, key);
      }
    }
    else {
      JS::RootedValue *value = new JS::RootedValue(GLOBAL_CX);
      JS_GetPropertyById(GLOBAL_CX, self->jsObject, id, value);
      JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));

      return pyTypeFactory(GLOBAL_CX, global, value)->getPyObject();
    }
  }
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_contains(JSObjectProxy *self, PyObject *key)
{
  JS::RootedId id(GLOBAL_CX);
  if (!keyToId(key, &id)) {
    // TODO (Caleb Aikens): raise exception here
    return -1; // key is not a str or int
  }
  JS::RootedValue *value = new JS::RootedValue(GLOBAL_CX);
  JS_GetPropertyById(GLOBAL_CX, self->jsObject, id, value);
  return value->isUndefined() ? 0 : 1;
}

int JSObjectProxyMethodDefinitions::JSObjectProxy_assign(JSObjectProxy *self, PyObject *key, PyObject *value)
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

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_richcompare(JSObjectProxy *self, PyObject *other, int op)
{
  if (op != Py_EQ && op != Py_NE) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  std::unordered_map<PyObject *, PyObject *> visited;

  bool isEqual = JSObjectProxy_richcompare_helper(self, other, visited);
  switch (op)
  {
  case Py_EQ:
    return PyBool_FromLong(isEqual);
  case Py_NE:
    return PyBool_FromLong(!isEqual);
  default:
    return NULL;
  }
}

bool JSObjectProxyMethodDefinitions::JSObjectProxy_richcompare_helper(JSObjectProxy *self, PyObject *other, std::unordered_map<PyObject *, PyObject *> &visited)
{
  if (!(Py_TYPE(other)->tp_iter) && !PySequence_Check(other) & !PyMapping_Check(other)) { // if other is not a container
    return false;
  }

  if ((visited.find((PyObject *)self) != visited.end() && visited[(PyObject *)self] == other) ||
      (visited.find((PyObject *)other) != visited.end() && visited[other] == (PyObject *)self)
  ) // if we've already compared these before, skip
  {
    return true;
  }

  visited.insert({{(PyObject *)self, other}});

  if (Py_TYPE((PyObject *)self) == Py_TYPE(other)) {
    JS::RootedValue selfVal(GLOBAL_CX, JS::ObjectValue(*self->jsObject));
    JS::RootedValue otherVal(GLOBAL_CX, JS::ObjectValue(*(*(JSObjectProxy *)other).jsObject));
    if (selfVal.asRawBits() == otherVal.asRawBits()) {
      return true;
    }
    bool *isEqual;
  }

  JS::RootedIdVector props(GLOBAL_CX);
  if (!js::GetPropertyKeys(GLOBAL_CX, self->jsObject, JSITER_OWNONLY | JSITER_HIDDEN, &props))
  {
    // @TODO (Caleb Aikens) raise exception here
    return NULL;
  }

  // iterate recursively through members of self and check for equality
  size_t length = props.length();
  for (size_t i = 0; i < length; i++)
  {
    JS::HandleId id = props[i];
    JS::RootedValue *key = new JS::RootedValue(GLOBAL_CX);
    key->setString(id.toString());

    JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
    PyObject *pyKey = pyTypeFactory(GLOBAL_CX, global, key)->getPyObject();
    PyObject *pyVal1 = PyObject_GetItem((PyObject *)self, pyKey);
    PyObject *pyVal2 = PyObject_GetItem((PyObject *)other, pyKey);
    if (!pyVal2) { // if other.key is NULL then not equal
      return false;
    }
    if (pyVal1 && Py_TYPE(pyVal1) == &JSObjectProxyType) { // if either subvalue is a JSObjectProxy, we need to pass around our visited map
      if (!JSObjectProxy_richcompare_helper((JSObjectProxy *)pyVal1, pyVal2, visited))
      {
        return false;
      }
    }
    else if (pyVal2 && Py_TYPE(pyVal2) == &JSObjectProxyType) {
      if (!JSObjectProxy_richcompare_helper((JSObjectProxy *)pyVal2, pyVal1, visited))
      {
        return false;
      }
    }
    else if (PyObject_RichCompare(pyVal1, pyVal2, Py_EQ) == Py_False) {
      return false;
    }
  }

  return true;
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_iter(JSObjectProxy *self) {
  // key iteration
  JSObjectIterProxy *iterator = PyObject_GC_New(JSObjectIterProxy, &JSObjectIterProxyType);
  if (iterator == NULL) {
    return NULL;
  }
  iterator->it.it_index = 0;
  iterator->it.reversed = false;
  iterator->it.kind = KIND_KEYS;
  Py_INCREF(self);
  iterator->it.di_dict = (PyDictObject *)self;
  iterator->it.props = new JS::RootedIdVector(GLOBAL_CX);
  // Get **enumerable** own properties
  if (!js::GetPropertyKeys(GLOBAL_CX, self->jsObject, JSITER_OWNONLY, iterator->it.props)) {
    return NULL;
  }
  PyObject_GC_Track(iterator);
  return (PyObject *)iterator;
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_repr(JSObjectProxy *self) {
  Py_ssize_t i = Py_ReprEnter((PyObject *)self);
  if (i != 0) {
    return i > 0 ? PyUnicode_FromString("{...}") : NULL;
  }

  Py_ssize_t selfLength = JSObjectProxy_length(self);

  if (selfLength == 0) {
    Py_ReprLeave((PyObject *)self);
    return PyUnicode_FromString("{}");
  }

  _PyUnicodeWriter writer;

  _PyUnicodeWriter_Init(&writer);

  writer.overallocate = 1;
  /* "{" + "1: 2" + ", 3: 4" * (len - 1) + "}" */
  writer.min_length = 1 + 4 + (2 + 4) * (selfLength - 1) + 1;

  PyObject *key = NULL, *value = NULL;

  JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));

  JS::RootedIdVector props(GLOBAL_CX);

  if (_PyUnicodeWriter_WriteChar(&writer, '{') < 0) {
    goto error;
  }

  /* Do repr() on each key+value pair, and insert ": " between them. Note that repr may mutate the dict. */

  // Get **enumerable** own properties
  if (!js::GetPropertyKeys(GLOBAL_CX, self->jsObject, JSITER_OWNONLY, &props)) {
    return NULL;
  }

  for (Py_ssize_t index = 0; index < selfLength; index++) {
    if (index > 0) {
      if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0) {
        goto error;
      }
    }

    JS::HandleId id = props[index];
    key = idToKey(GLOBAL_CX, id);

    // Prevent repr from deleting key or value during key format.
    Py_INCREF(key);

    PyObject *s = PyObject_Repr(key);

    if (s == NULL) {
      Py_DECREF(s);
      goto error;
    }

    int res = _PyUnicodeWriter_WriteStr(&writer, s);

    if (res < 0) {
      goto error;
    }

    if (_PyUnicodeWriter_WriteASCIIString(&writer, ": ", 2) < 0) {
      goto error;
    }

    JS::RootedValue *elementVal = new JS::RootedValue(GLOBAL_CX);
    JS_GetPropertyById(GLOBAL_CX, self->jsObject, id, elementVal);

    if (&elementVal->toObject() == self->jsObject.get()) {
      value = (PyObject *)self;
    } else {
      value = pyTypeFactory(GLOBAL_CX, global, elementVal)->getPyObject();
    }

    Py_INCREF(value);
    s = PyObject_Repr(value);

    if (s == NULL) {
      Py_DECREF(s);
      goto error;
    }

    res = _PyUnicodeWriter_WriteStr(&writer, s);
    Py_DECREF(s);
    if (res < 0) {
      goto error;
    }

    Py_CLEAR(key);
    Py_CLEAR(value);
  }

  writer.overallocate = 0;
  if (_PyUnicodeWriter_WriteChar(&writer, '}') < 0) {
    goto error;
  }

  Py_ReprLeave((PyObject *)self);
  return _PyUnicodeWriter_Finish(&writer);

error:
  Py_ReprLeave((PyObject *)self);
  _PyUnicodeWriter_Dealloc(&writer);
  Py_XDECREF(key);
  Py_XDECREF(value);
  return NULL;
}

// private
static int mergeFromSeq2(JSObjectProxy *self, PyObject *seq2) {
  PyObject *it;         /* iter(seq2) */
  Py_ssize_t i;         /* index into seq2 of current element */
  PyObject *item;       /* seq2[i] */
  PyObject *fast;       /* item as a 2-tuple or 2-list */

  it = PyObject_GetIter(seq2);
  if (it == NULL)
    return -1;

  for (i = 0;; ++i) {
    PyObject *key, *value;
    Py_ssize_t n;

    fast = NULL;
    item = PyIter_Next(it);
    if (item == NULL) {
      if (PyErr_Occurred())
        goto Fail;
      break;
    }

    /* Convert item to sequence, and verify length 2. */
    fast = PySequence_Fast(item, "");
    if (fast == NULL) {
      if (PyErr_ExceptionMatches(PyExc_TypeError))
        PyErr_Format(PyExc_TypeError,
          "cannot convert dictionary update "
          "sequence element #%zd to a sequence",
          i);
      goto Fail;
    }
    n = PySequence_Fast_GET_SIZE(fast);
    if (n != 2) {
      PyErr_Format(PyExc_ValueError,
        "dictionary update sequence element #%zd "
        "has length %zd; 2 is required",
        i, n);
      goto Fail;
    }

    /* Update/merge with this (key, value) pair. */
    key = PySequence_Fast_GET_ITEM(fast, 0);
    value = PySequence_Fast_GET_ITEM(fast, 1);
    Py_INCREF(key);
    Py_INCREF(value);

    if (JSObjectProxyMethodDefinitions::JSObjectProxy_assign(self, key, value) < 0) {
      Py_DECREF(key);
      Py_DECREF(value);
      goto Fail;
    }

    Py_DECREF(key);
    Py_DECREF(value);
    Py_DECREF(fast);
    Py_DECREF(item);
  }

  i = 0;
  goto Return;
Fail:
  Py_XDECREF(item);
  Py_XDECREF(fast);
  i = -1;
Return:
  Py_DECREF(it);
  return Py_SAFE_DOWNCAST(i, Py_ssize_t, int);
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_or(JSObjectProxy *self, PyObject *other) {
  #if PY_VERSION_HEX < 0x03090000
  // | is not supported on dicts in python3.8 or less, so only allow if both
  // operands are JSObjectProxy
  if (!PyObject_TypeCheck(self, &JSObjectProxyType) || !PyObject_TypeCheck(other, &JSObjectProxyType)) {
    Py_RETURN_NOTIMPLEMENTED;
  }
  #endif
  if (!PyDict_Check(self) || !PyDict_Check(other)) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  if (!PyObject_TypeCheck(self, &JSObjectProxyType) && PyObject_TypeCheck(other, &JSObjectProxyType)) {
    return PyDict_Type.tp_as_number->nb_or((PyObject *)&(self->dict), other);
  }
  else {
    JS::Rooted<JS::ValueArray<3>> args(GLOBAL_CX);
    args[0].setObjectOrNull(JS_NewPlainObject(GLOBAL_CX));
    args[1].setObjectOrNull(self->jsObject);  // this is null is left operand is real dict
    JS::RootedValue jValueOther(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, other));
    args[2].setObject(jValueOther.toObject());

    JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));

    // call Object.assign
    JS::RootedValue Object(GLOBAL_CX);
    if (!JS_GetProperty(GLOBAL_CX, *global, "Object", &Object)) {
      PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSObjectProxyType.tp_name);
      return NULL;
    }

    JS::RootedObject rootedObject(GLOBAL_CX, Object.toObjectOrNull());
    JS::RootedValue *ret = new JS::RootedValue(GLOBAL_CX);

    if (!JS_CallFunctionName(GLOBAL_CX, rootedObject, "assign", args, ret)) {
      PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSObjectProxyType.tp_name);
      return NULL;
    }
    return pyTypeFactory(GLOBAL_CX, global, ret)->getPyObject();
  }
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_ior(JSObjectProxy *self, PyObject *other) {
  if (PyDict_Check(other)) {
    JS::Rooted<JS::ValueArray<2>> args(GLOBAL_CX);
    args[0].setObjectOrNull(self->jsObject);
    JS::RootedValue jValueOther(GLOBAL_CX, jsTypeFactory(GLOBAL_CX, other));
    args[1].setObject(jValueOther.toObject());

    JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));

    // call Object.assign
    JS::RootedValue Object(GLOBAL_CX);
    if (!JS_GetProperty(GLOBAL_CX, *global, "Object", &Object)) {
      PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSObjectProxyType.tp_name);
      return NULL;
    }

    JS::RootedObject rootedObject(GLOBAL_CX, Object.toObjectOrNull());
    JS::RootedValue ret(GLOBAL_CX);
    if (!JS_CallFunctionName(GLOBAL_CX, rootedObject, "assign", args, &ret)) {
      PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSObjectProxyType.tp_name);
      return NULL;
    }
  }
  else {
    if (mergeFromSeq2(self, other) < 0) {
      return NULL;
    }
  }

  Py_INCREF(self);
  return (PyObject *)self;
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_get_method(JSObjectProxy *self, PyObject *const *args, Py_ssize_t nargs) {
  PyObject *key;
  PyObject *default_value = Py_None;

  if (!_PyArg_CheckPositional("get", nargs, 1, 2)) {
    return NULL;
  }
  key = args[0];
  if (nargs < 2) {
    goto skip_optional;
  }

  default_value = args[1];

skip_optional:

  PyObject *value = JSObjectProxy_get(self, key);
  if (value == Py_None) {
    value = default_value;
  }
  Py_XINCREF(value);
  return value;
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_setdefault_method(JSObjectProxy *self, PyObject *const *args, Py_ssize_t nargs) {
  PyObject *key;
  PyObject *default_value = Py_None;

  if (!_PyArg_CheckPositional("setdefault", nargs, 1, 2)) {
    return NULL;
  }
  key = args[0];
  if (nargs < 2) {
    goto skip_optional;
  }

  default_value = args[1];

skip_optional:

  PyObject* value = JSObjectProxy_get(self, key);
  if (value == Py_None) {
    JSObjectProxy_assign(self, key, default_value);
    Py_XINCREF(default_value);
    return default_value;
  }

  Py_XINCREF(value);
  return value;
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_pop_method(JSObjectProxy *self, PyObject *const *args, Py_ssize_t nargs) {
  PyObject *key;
  PyObject *default_value = NULL;

  if (!_PyArg_CheckPositional("pop", nargs, 1, 2)) {
    return NULL;
  }
  key = args[0];
  if (nargs < 2) {
    goto skip_optional;
  }
  default_value = args[1];

skip_optional:
  JS::RootedId id(GLOBAL_CX);
  if (!keyToId(key, &id)) {
    // TODO (Caleb Aikens): raise exception here  PyObject *seq = PyTuple_New(length);
    return NULL;
  }

  JS::RootedValue *value = new JS::RootedValue(GLOBAL_CX);
  JS_GetPropertyById(GLOBAL_CX, self->jsObject, id, value);
  if (value->isUndefined()) {
    if (default_value != NULL) {
      Py_INCREF(default_value);
      return default_value;
    }
    _PyErr_SetKeyError(key);
    return NULL;
  }
  else {
    JS::ObjectOpResult ignoredResult;
    JS_DeletePropertyById(GLOBAL_CX, self->jsObject, id, ignoredResult);

    JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));
    return pyTypeFactory(GLOBAL_CX, global, value)->getPyObject();
  }
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_clear_method(JSObjectProxy *self) {
  JS::RootedIdVector props(GLOBAL_CX);
  if (!js::GetPropertyKeys(GLOBAL_CX, self->jsObject, JSITER_OWNONLY, &props))
  {
    // @TODO (Caleb Aikens) raise exception here
    return NULL;
  }

  JS::ObjectOpResult ignoredResult;
  size_t length = props.length();
  for (size_t index = 0; index < length; index++) {
    JS_DeletePropertyById(GLOBAL_CX, self->jsObject, props[index], ignoredResult);
  }

  Py_RETURN_NONE;
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_copy_method(JSObjectProxy *self) {
  JS::Rooted<JS::ValueArray<2>> args(GLOBAL_CX);
  args[0].setObjectOrNull(JS_NewPlainObject(GLOBAL_CX));
  args[1].setObjectOrNull(self->jsObject);

  JS::RootedObject *global = new JS::RootedObject(GLOBAL_CX, JS::GetNonCCWObjectGlobal(self->jsObject));

  // call Object.assign
  JS::RootedValue Object(GLOBAL_CX);
  if (!JS_GetProperty(GLOBAL_CX, *global, "Object", &Object)) {
    PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSObjectProxyType.tp_name);
    return NULL;
  }

  JS::RootedObject rootedObject(GLOBAL_CX, Object.toObjectOrNull());
  JS::RootedValue *ret = new JS::RootedValue(GLOBAL_CX);
  if (!JS_CallFunctionName(GLOBAL_CX, rootedObject, "assign", args, ret)) {
    PyErr_Format(PyExc_SystemError, "%s JSAPI call failed", JSObjectProxyType.tp_name);
    return NULL;
  }
  return pyTypeFactory(GLOBAL_CX, global, ret)->getPyObject();
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_update_method(JSObjectProxy *self, PyObject *args, PyObject *kwds) {
  PyObject *arg = NULL;
  int result = 0;

  if (!PyArg_UnpackTuple(args, "update", 0, 1, &arg)) {
    return NULL;
  }
  else if (arg != NULL) {
    if (PyDict_CheckExact(arg) || PyObject_TypeCheck(arg, &JSObjectProxyType)) {
      JSObjectProxyMethodDefinitions::JSObjectProxy_ior((JSObjectProxy *)self, arg);
      result = 0;
    } else { // iterable
      result = mergeFromSeq2((JSObjectProxy *)self, arg);
      if (result < 0) {
        return NULL;
      }
    }
  }

  if (result == 0 && kwds != NULL) {
    if (PyArg_ValidateKeywordArguments(kwds)) {
      JSObjectProxyMethodDefinitions::JSObjectProxy_ior((JSObjectProxy *)self, kwds);
    }
  }
  Py_RETURN_NONE;
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_keys_method(JSObjectProxy *self) {
  return _PyDictView_New((PyObject *)self, &JSObjectKeysProxyType);
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_values_method(JSObjectProxy *self) {
  return _PyDictView_New((PyObject *)self, &JSObjectValuesProxyType);
}

PyObject *JSObjectProxyMethodDefinitions::JSObjectProxy_items_method(JSObjectProxy *self) {
  return _PyDictView_New((PyObject *)self, &JSObjectItemsProxyType);
}