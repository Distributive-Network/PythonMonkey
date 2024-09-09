/**
 * @file PyListProxyHandler.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS proxy objects for Lists
 * @date 2023-12-01
 *
 * @copyright Copyright (c) 2023-2024 Distributive Corp.
 *
 */

#include "include/PyListProxyHandler.hh"
#include "include/PyBaseProxyHandler.hh"

#include "include/jsTypeFactory.hh"
#include "include/JSArrayProxy.hh"
#include "include/JSFunctionProxy.hh"
#include "include/pyTypeFactory.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Conversions.h>
#include <js/Proxy.h>
#include <js/Symbol.h>
#include <js/friend/ErrorMessages.h>

#include <Python.h>



const char PyListProxyHandler::family = 0;

// private util
// if function is a proxy for a python method, mutate it into a new python method bound to thisObject
static bool makeNewPyMethod(JSContext *cx, JS::MutableHandleValue function, JS::HandleObject thisObject) {
  if (!JS_IsNativeFunction(&(function.toObject()), callPyFunc)) {
    return true; // we don't need to mutate function if it is not a proxy for a python function
  }

  PyObject *method = (PyObject *)js::GetFunctionNativeReserved(&(function.toObject()), 0).toPrivate();
  if (!PyMethod_Check(method)) {
    PyErr_Format(PyExc_TypeError, "unbound python functions do not have a 'self' to bind");
    return false;
  }

  PyObject *func = PyMethod_Function(method);
  JS::RootedValue thisValue(cx);
  thisValue.setObject(*thisObject);
  PyObject *newSelf = pyTypeFactory(cx, thisValue);
  function.set(jsTypeFactory(cx, PyMethod_New(func, newSelf)));
  Py_DECREF(newSelf);

  return true;
}

static bool array_reverse(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  if (PyList_GET_SIZE(self) > 1) {
    if (PyList_Reverse(self) < 0) {
      return false;
    }
  }

  // return ref to self
  args.rval().set(jsTypeFactory(cx, self));
  return true;
}

static bool array_pop(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  if (PyList_GET_SIZE(self) == 0) {
    args.rval().setUndefined();
    return true;
  }

  PyObject *result = PyObject_CallMethod(self, "pop", NULL);

  if (!result) {
    PyErr_Clear();
    args.rval().setUndefined();
    return true;
  }

  args.rval().set(jsTypeFactory(cx, result));
  Py_DECREF(result);
  return true;
}

static bool array_push(JSContext *cx, unsigned argc, JS::Value *vp) { // surely the function name is in there...review JSAPI examples
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  unsigned numArgs = args.length();
  JS::RootedValue elementVal(cx);
  for (unsigned index = 0; index < numArgs; index++) {
    elementVal.set(args[index].get());
    PyObject *value = pyTypeFactory(cx, elementVal);
    if (PyList_Append(self, value) < 0) {
      Py_DECREF(value);
      return false;
    }
    Py_DECREF(value);
  }

  args.rval().setInt32(PyList_GET_SIZE(self));
  return true;
}

static bool array_shift(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  Py_ssize_t selfSize = PyList_GET_SIZE(self);

  if (selfSize == 0) {
    args.rval().setUndefined();
    return true;
  }

  PyObject *result = PyList_GetItem(self, 0);
  if (!result) {
    return false;
  }
  if (PySequence_DelItem(self, 0) < 0) {
    return false;
  }

  args.rval().set(jsTypeFactory(cx, result));
  return true;
}

static bool array_unshift(JSContext *cx, unsigned argc, JS::Value *vp) { // surely the function name is in there...review JSAPI examples
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::RootedValue elementVal(cx);
  for (int index = args.length() - 1; index >= 0; index--) {
    elementVal.set(args[index].get());
    PyObject *value = pyTypeFactory(cx, elementVal);
    if (PyList_Insert(self, 0, value) < 0) {
      Py_DECREF(value);
      return false;
    }
    Py_DECREF(value);
  }

  args.rval().setInt32(PyList_GET_SIZE(self));
  return true;
}

// private util
static inline uint64_t normalizeSliceTerm(int64_t value, uint64_t length) {
  if (value < 0) {
    value += length;
    if (value < 0) {
      return 0;
    }
  }
  else if (double(value) > double(length)) {
    return length;
  }
  return value;
}

static bool array_slice(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "slice", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  Py_ssize_t selfLength = PyList_GET_SIZE(self);

  uint64_t start = 0;
  uint64_t stop = selfLength;
  if (args.length() > 0) {
    int64_t d;

    if (!JS::ToInt64(cx, args[0], &d)) {
      return false;
    }

    start = normalizeSliceTerm(d, selfLength);

    if (args.hasDefined(1)) {
      if (!JS::ToInt64(cx, args[1], &d)) {
        return false;
      }

      stop = normalizeSliceTerm(d, selfLength);
    }
  }

  PyObject *result = PyList_GetSlice(self, (Py_ssize_t)start, (Py_ssize_t)stop);
  if (!result) {
    return false;
  }

  args.rval().set(jsTypeFactory(cx, result));
  Py_DECREF(result);
  return true;
}

static bool array_indexOf(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "indexOf", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  Py_ssize_t selfLength = PyList_GET_SIZE(self);

  if (selfLength == 0) {
    args.rval().setInt32(-1);
    return true;
  }

  uint64_t start = 0;
  if (args.length() > 1) {
    int64_t n;
    if (!JS::ToInt64(cx, args[1], &n)) {
      return false;
    }

    if (n >= selfLength) {
      args.rval().setInt32(-1);
      return true;
    }

    if (n >= 0) {
      start = uint64_t(n);
    }
    else {
      int64_t d = selfLength + n;
      if (d >= 0) {
        start = d;
      }
    }
  }

  JS::RootedValue elementVal(cx, args[0].get());
  PyObject *value = pyTypeFactory(cx, elementVal);
  PyObject *result = PyObject_CallMethod(self, "index", "Oi", value, start);
  Py_DECREF(value);

  if (!result) {
    PyErr_Clear();
    args.rval().setInt32(-1);
    return true;
  }

  args.rval().set(jsTypeFactory(cx, result));
  Py_DECREF(result);
  return true;
}

static bool array_splice(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  uint64_t selfLength = (uint64_t)PyList_GET_SIZE(self);

  int64_t relativeStart;
  if (!JS::ToInt64(cx, args.get(0), &relativeStart)) {
    return false;
  }

  /* actualStart is the index after which elements will be
      deleted and/or new elements will be added */
  uint64_t actualStart;
  if (relativeStart < 0) {
    actualStart = uint64_t(std::max(double(selfLength) + relativeStart, 0.0));
  } else {
    actualStart = uint64_t(std::min(double(relativeStart), double(selfLength)));
  }

  unsigned int argsLength = args.length();

  /* insertCount is the number of elements being added */
  uint32_t insertCount;
  if (argsLength < 2) {
    insertCount = 0;
  }
  else {
    insertCount = argsLength - 2;
  }

  /* actualDeleteCount is the number of elements being deleted */
  uint64_t actualDeleteCount;
  if (argsLength < 1) {
    actualDeleteCount = 0;
  }
  else if (argsLength < 2) {
    actualDeleteCount = selfLength - actualStart;
  }
  else {
    int64_t deleteCount;
    if (!JS::ToInt64(cx, args.get(1), &deleteCount)) {
      return false;
    }

    actualDeleteCount = uint64_t(std::min(std::max(0.0, double(deleteCount)), double(selfLength - actualStart)));
  }

  // get deleted items for return value
  PyObject *deleted = PyList_GetSlice(self, actualStart, actualStart + actualDeleteCount);
  if (!deleted) {
    return false;
  }

  // build list for SetSlice call
  PyObject *inserted = PyList_New(insertCount);
  if (!inserted) {
    return false;
  }

  JS::RootedValue elementVal(cx);
  for (int index = 0; index < insertCount; index++) {
    elementVal.set(args[index + 2].get());
    PyObject *value = pyTypeFactory(cx, elementVal);
    if (PyList_SetItem(inserted, index, value) < 0) {
      return false;
    }
  }

  if (PyList_SetSlice(self, actualStart, actualStart + actualDeleteCount, inserted) < 0) {
    return false;
  }

  args.rval().set(jsTypeFactory(cx, deleted));
  Py_DECREF(deleted);
  return true;
}

static bool array_fill(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "fill", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  uint64_t selfLength = (uint64_t)PyList_GET_SIZE(self);

  unsigned int argsLength = args.length();

  int64_t relativeStart;
  if (argsLength > 1) {
    if (!JS::ToInt64(cx, args.get(1), &relativeStart)) {
      return false;
    }
  } else {
    relativeStart = 0;
  }

  uint64_t actualStart;
  if (relativeStart < 0) {
    actualStart = uint64_t(std::max(double(selfLength) + relativeStart, 0.0));
  } else {
    actualStart = uint64_t(std::min(double(relativeStart), double(selfLength)));
  }

  int64_t relativeEnd;
  if (argsLength > 2) {
    if (!JS::ToInt64(cx, args.get(2), &relativeEnd)) {
      return false;
    }
  } else {
    relativeEnd = selfLength;
  }

  uint64_t actualEnd;
  if (relativeEnd < 0) {
    actualEnd = uint64_t(std::max(double(selfLength) + relativeEnd, 0.0));
  } else {
    actualEnd = uint64_t(std::min(double(relativeEnd), double(selfLength)));
  }

  JS::RootedValue fillValue(cx, args[0].get());
  PyObject *fillValueItem = pyTypeFactory(cx, fillValue);
  bool setItemCalled = false;
  for (int index = actualStart; index < actualEnd; index++) {
    setItemCalled = true;
    if (PyList_SetItem(self, index, fillValueItem) < 0) {
      return false;
    }
  }

  if (!setItemCalled) {
    Py_DECREF(fillValueItem);
  }

  // return ref to self
  args.rval().set(jsTypeFactory(cx, self));
  return true;
}

static bool array_copyWithin(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  int64_t selfLength = (uint64_t)PyList_GET_SIZE(self);

  unsigned int argsLength = args.length();

  int64_t relativeTarget;
  if (argsLength > 0) {
    if (!JS::ToInt64(cx, args.get(0), &relativeTarget)) {
      return false;
    }
  } else {
    relativeTarget = 0;
  }

  int64_t actualTarget;
  if (relativeTarget < 0) {
    actualTarget = int64_t(std::max(double(selfLength) + relativeTarget, 0.0));
  } else {
    actualTarget = int64_t(std::min(double(relativeTarget), double(selfLength)));
  }

  int64_t relativeStart;
  if (argsLength > 1) {
    if (!JS::ToInt64(cx, args.get(1), &relativeStart)) {
      return false;
    }
  } else {
    relativeStart = 0;
  }

  int64_t actualStart;
  if (relativeStart < 0) {
    actualStart = int64_t(std::max(double(selfLength) + relativeStart, 0.0));
  } else {
    actualStart = int64_t(std::min(double(relativeStart), double(selfLength)));
  }

  int64_t relativeEnd;
  if (argsLength > 2) {
    if (!JS::ToInt64(cx, args.get(2), &relativeEnd)) {
      return false;
    }
  } else {
    relativeEnd = selfLength;
  }

  int64_t actualEnd;
  if (relativeEnd < 0) {
    actualEnd = int64_t(std::max(double(selfLength) + relativeEnd, 0.0));
  } else {
    actualEnd = int64_t(std::min(double(relativeEnd), double(selfLength)));
  }

  int64_t count = int64_t(std::min(actualEnd - actualStart, selfLength - actualTarget));

  if (actualStart < actualTarget && actualTarget < actualStart + count) {
    actualStart = actualStart + count - 1;
    actualTarget = actualTarget + count - 1;

    while (count > 0) {
      PyObject *itemStart = PyList_GetItem(self, actualStart);
      if (PyList_SetItem(self, actualTarget, itemStart) < 0) {
        return false;
      }

      actualStart--;
      actualTarget--;
      count--;
    }
  } else {
    while (count > 0) {
      PyObject *itemStart = PyList_GetItem(self, actualStart);
      if (PyList_SetItem(self, actualTarget, itemStart) < 0) {
        return false;
      }

      actualStart++;
      actualTarget++;
      count--;
    }
  }

  // return ref to self
  args.rval().set(jsTypeFactory(cx, self));
  return true;
}

static bool array_concat(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  Py_ssize_t selfSize = PyList_GET_SIZE(self);

  PyObject *result = PyList_New(selfSize);

  for (Py_ssize_t index = 0; index < selfSize; index++) {
    PyList_SetItem(result, index, PyList_GetItem(self, index));
  }

  unsigned numArgs = args.length();
  JS::RootedValue elementVal(cx);
  for (unsigned index = 0; index < numArgs; index++) {
    elementVal.set(args[index].get());

    PyObject *item = pyTypeFactory(cx, elementVal);
    if (PyObject_TypeCheck(item, &JSArrayProxyType)) {
      // flatten the array only a depth 1
      Py_ssize_t itemLength = JSArrayProxyMethodDefinitions::JSArrayProxy_length((JSArrayProxy *)item);
      for (Py_ssize_t flatIndex = 0; flatIndex < itemLength; flatIndex++) {
        if (!JS_GetElement(cx, *(((JSArrayProxy *)item)->jsArray), flatIndex, &elementVal)) {
          Py_DECREF(item);
          return false;
        }
        PyObject *value = pyTypeFactory(cx, elementVal);
        if (PyList_Append(result, value) < 0) {
          Py_DECREF(item);
          Py_DECREF(value);
          return false;
        }
        Py_DECREF(value);
      }
    }
    else if (PyObject_TypeCheck(item, &PyList_Type)) {
      // flatten the array only at depth 1
      Py_ssize_t itemLength = PyList_GET_SIZE(item);
      for (Py_ssize_t flatIndex = 0; flatIndex < itemLength; flatIndex++) {
        if (PyList_Append(result, PyList_GetItem(item, flatIndex)) < 0) {
          Py_DECREF(item);
          return false;
        }
      }
    }
    else {
      PyObject *value = pyTypeFactory(cx, elementVal);
      if (PyList_Append(result, value) < 0) {
        Py_DECREF(item);
        Py_DECREF(value);
        return false;
      }
      Py_DECREF(value);
    }

    Py_DECREF(item);
  }

  args.rval().set(jsTypeFactory(cx, result));
  Py_DECREF(result);
  return true;
}

static bool array_lastIndexOf(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "lastIndexOf", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  Py_ssize_t selfLength = PyList_GET_SIZE(self);

  if (selfLength == 0) {
    args.rval().setInt32(-1);
    return true;
  }

  uint64_t start = selfLength - 1;
  if (args.length() > 1) {
    int64_t n;
    if (!JS::ToInt64(cx, args[1], &n)) {
      return false;
    }

    if (n < 0) {
      double d = double(selfLength) + n;
      if (d < 0) {
        args.rval().setInt32(-1);
        return true;
      }
      start = uint64_t(d);
    } else if (n < double(start)) {
      start = uint64_t(n);
    }
  }

  JS::RootedValue elementVal(cx, args[0].get());
  PyObject *element = pyTypeFactory(cx, elementVal);
  for (int64_t index = start; index >= 0; index--) {
    PyObject *item = PyList_GetItem(self, index);
    Py_INCREF(item);
    int cmp = PyObject_RichCompareBool(item, element, Py_EQ);
    Py_DECREF(item);
    if (cmp < 0) {
      Py_XDECREF(element);
      return false;
    }
    else if (cmp == 1) {
      Py_XDECREF(element);
      args.rval().setInt32(index);
      return true;
    }
  }
  Py_XDECREF(element);

  args.rval().setInt32(-1);
  return true;
}

static bool array_includes(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "includes", 1)) {
    return false;
  }

  if (!array_indexOf(cx, argc, vp)) {
    return false;
  }

  args.rval().setBoolean(args.rval().get().toInt32() >= 0 ? true : false);
  return true;
}

static bool array_forEach(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "forEach", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::Value callbackfn = args[0].get();

  if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
    JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "forEach: callback");
    return false;
  }

  JS::RootedValue selfValue(cx, jsTypeFactory(cx, self));
  JS::RootedValue callBack(cx, callbackfn);

  JS::Rooted<JS::ValueArray<3>> jArgs(cx);
  JS::RootedValue rval(cx);

  Py_ssize_t len = PyList_GET_SIZE(self);

  JS::RootedObject rootedThisArg(cx);

  if (args.length() > 1) {
    JS::Value thisArg = args[1].get();
    if (!thisArg.isObjectOrNull()) {
      JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_OBJORNULL, "'this' argument");
      return false;
    }
    // TODO support null, currently gets TypeError
    rootedThisArg.set(thisArg.toObjectOrNull());
    // check if callback is a PyMethod, need to make a new method bound to thisArg
    if (!makeNewPyMethod(cx, &callBack, rootedThisArg)) {
      return false;
    }
  }
  else {
    rootedThisArg.set(nullptr);
  }

  for (Py_ssize_t index = 0; index < len; index++) {
    jArgs[0].set(jsTypeFactory(cx, PyList_GetItem(self, index)));
    jArgs[1].setInt32(index);
    jArgs[2].set(selfValue);

    if (!JS_CallFunctionValue(cx, rootedThisArg, callBack, jArgs, &rval)) {
      return false;
    }
  }

  args.rval().setUndefined();
  return true;
}

static bool array_map(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "map", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::Value callbackfn = args[0].get();

  if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
    // Decompiling the faulty arg is not accessible through the JSAPI so we do the best effort for the error message
    JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "map: callback");
    return false;
  }

  Py_ssize_t len = PyList_GET_SIZE(self);

  JSObject *retArray = JS::NewArrayObject(cx, len);
  JS::RootedObject rootedRetArray(cx, retArray);

  JS::RootedValue selfValue(cx, jsTypeFactory(cx, self));
  JS::RootedValue callBack(cx, callbackfn);

  JS::Rooted<JS::ValueArray<3>> jArgs(cx);
  JS::RootedValue rval(cx);

  JS::RootedObject rootedThisArg(cx);
  if (args.length() > 1) {
    JS::Value thisArg = args[1].get();
    if (!thisArg.isObjectOrNull()) {
      JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_OBJORNULL, "'this' argument");
      return false;
    }

    // TODO support null, currently gets TypeError
    rootedThisArg.set(thisArg.toObjectOrNull());
    // check if callback is a PyMethod, need to make a new method bound to thisArg
    if (!makeNewPyMethod(cx, &callBack, rootedThisArg)) {
      return false;
    }
  }
  else {
    rootedThisArg.set(nullptr);
  }

  for (Py_ssize_t index = 0; index < len; index++) {
    jArgs[0].set(jsTypeFactory(cx, PyList_GetItem(self, index)));
    jArgs[1].setInt32(index);
    jArgs[2].set(selfValue);

    if (!JS_CallFunctionValue(cx, rootedThisArg, callBack, jArgs, &rval)) {
      return false;
    }

    JS_SetElement(cx, rootedRetArray, index, rval);
  }

  args.rval().setObject(*retArray);
  return true;
}

static bool array_filter(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "filter", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::Value callbackfn = args[0].get();

  if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
    JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "filter: callback");
    return false;
  }

  JS::RootedValue selfValue(cx, jsTypeFactory(cx, self));
  JS::RootedValue callBack(cx, callbackfn);

  JS::Rooted<JS::ValueArray<3>> jArgs(cx);
  JS::RootedValue rval(cx);

  JS::RootedValueVector retVector(cx);

  JS::RootedObject rootedThisArg(cx);
  if (args.length() > 1) {
    JS::Value thisArg = args[1].get();
    if (!thisArg.isObjectOrNull()) {
      JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_OBJORNULL, "'this' argument");
      return false;
    }

    // TODO support null, currently gets TypeError
    rootedThisArg.set(thisArg.toObjectOrNull());
    // check if callback is a PyMethod, need to make a new method bound to thisArg
    if (!makeNewPyMethod(cx, &callBack, rootedThisArg)) {
      return false;
    }
  }
  else {
    rootedThisArg.set(nullptr);
  }

  Py_ssize_t len = PyList_GET_SIZE(self);
  for (Py_ssize_t index = 0, toIndex = 0; index < len; index++) {
    JS::Value item = jsTypeFactory(cx, PyList_GetItem(self, index));
    jArgs[0].set(item);
    jArgs[1].setInt32(index);
    jArgs[2].set(selfValue);

    if (!JS_CallFunctionValue(cx, rootedThisArg, callBack, jArgs, &rval)) {
      return false;
    }

    if (rval.toBoolean()) {
      if (!retVector.append(item)) {
        return false;
      }
    }
  }

  JS::HandleValueArray jsValueArray(retVector);
  JSObject *retArray = JS::NewArrayObject(cx, jsValueArray);

  args.rval().setObject(*retArray);
  return true;
}

static bool array_reduce(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "reduce", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::Value callbackfn = args[0].get();

  if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
    JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "reduce: callback");
    return false;
  }

  JS::RootedValue selfValue(cx, jsTypeFactory(cx, self));
  JS::RootedValue callBack(cx, callbackfn);

  JS::Rooted<JS::ValueArray<4>> jArgs(cx);
  JS::RootedValue *accumulator;

  Py_ssize_t len = PyList_GET_SIZE(self);

  if (args.length() > 1) {
    accumulator = new JS::RootedValue(cx, args[1].get());
  }
  else {
    if (len == 0) {
      JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_EMPTY_ARRAY_REDUCE);
      return false;
    }
    accumulator = new JS::RootedValue(cx, jsTypeFactory(cx, PyList_GetItem(self, 0)));
  }

  for (Py_ssize_t index = args.length() > 1 ? 0 : 1; index < len; index++) {
    jArgs[0].set(*accumulator);
    jArgs[1].set(jsTypeFactory(cx, PyList_GetItem(self, index)));
    jArgs[2].setInt32(index);
    jArgs[3].set(selfValue);

    if (!JS_CallFunctionValue(cx, nullptr, callBack, jArgs, accumulator)) {
      delete accumulator;
      return false;
    }
  }

  args.rval().set(accumulator->get());
  delete accumulator;
  return true;
}

static bool array_reduceRight(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "reduceRight", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::Value callbackfn = args[0].get();

  if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
    JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "reduceRight: callback");
    return false;
  }

  JS::RootedValue selfValue(cx, jsTypeFactory(cx, self));
  JS::RootedValue callBack(cx, callbackfn);

  JS::Rooted<JS::ValueArray<4>> jArgs(cx);
  JS::RootedValue accumulator(cx);

  Py_ssize_t len = PyList_GET_SIZE(self);

  if (args.length() > 1) {
    accumulator.set(args[1].get());
  }
  else {
    if (len == 0) {
      JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_EMPTY_ARRAY_REDUCE);
      return false;
    }
    accumulator.set(jsTypeFactory(cx, PyList_GetItem(self, len - 1)));
  }

  for (int64_t index = args.length() > 1 ? len - 1 : len - 2; index >= 0; index--) {
    jArgs[0].set(accumulator);
    jArgs[1].set(jsTypeFactory(cx, PyList_GetItem(self, index)));
    jArgs[2].setInt32(index);
    jArgs[3].set(selfValue);

    if (!JS_CallFunctionValue(cx, nullptr, callBack, jArgs, &accumulator)) {
      return false;
    }
  }

  args.rval().set(accumulator.get());
  return true;
}

static bool array_some(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "some", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::Value callbackfn = args[0].get();

  if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
    JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "some: callback");
    return false;
  }

  JS::RootedValue selfValue(cx, jsTypeFactory(cx, self));
  JS::RootedValue callBack(cx, callbackfn);

  JS::Rooted<JS::ValueArray<3>> jArgs(cx);
  JS::RootedValue rval(cx);

  JS::RootedObject rootedThisArg(cx);
  if (args.length() > 1) {
    JS::Value thisArg = args[1].get();
    if (!thisArg.isObjectOrNull()) {
      JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_OBJORNULL, "'this' argument");
      return false;
    }

    // TODO support null, currently gets TypeError
    rootedThisArg.set(thisArg.toObjectOrNull());
    // check if callback is a PyMethod, need to make a new method bound to thisArg
    if (!makeNewPyMethod(cx, &callBack, rootedThisArg)) {
      return false;
    }
  }
  else {
    rootedThisArg.set(nullptr);
  }

  Py_ssize_t len = PyList_GET_SIZE(self);
  for (Py_ssize_t index = 0, toIndex = 0; index < len; index++) {
    jArgs[0].set(jsTypeFactory(cx, PyList_GetItem(self, index)));
    jArgs[1].setInt32(index);
    jArgs[2].set(selfValue);

    if (!JS_CallFunctionValue(cx, rootedThisArg, callBack, jArgs, &rval)) {
      return false;
    }

    if (rval.toBoolean()) {
      args.rval().setBoolean(true);
      return true;
    }
  }

  args.rval().setBoolean(false);
  return true;
}

static bool array_every(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "every", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::Value callbackfn = args[0].get();

  if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
    JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "every: callback");
    return false;
  }

  JS::RootedValue selfValue(cx, jsTypeFactory(cx, self));
  JS::RootedValue callBack(cx, callbackfn);

  JS::Rooted<JS::ValueArray<3>> jArgs(cx);
  JS::RootedValue rval(cx);

  JS::RootedObject rootedThisArg(cx);
  if (args.length() > 1) {
    JS::Value thisArg = args[1].get();
    if (!thisArg.isObjectOrNull()) {
      JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_OBJORNULL, "'this' argument");
      return false;
    }

    // TODO support null, currently gets TypeError
    rootedThisArg.set(thisArg.toObjectOrNull());
    // check if callback is a PyMethod, need to make a new method bound to thisArg
    if (!makeNewPyMethod(cx, &callBack, rootedThisArg)) {
      return false;
    }
  }
  else {
    rootedThisArg.set(nullptr);
  }

  Py_ssize_t len = PyList_GET_SIZE(self);
  for (Py_ssize_t index = 0, toIndex = 0; index < len; index++) {
    jArgs[0].set(jsTypeFactory(cx, PyList_GetItem(self, index)));
    jArgs[1].setInt32(index);
    jArgs[2].set(selfValue);

    if (!JS_CallFunctionValue(cx, rootedThisArg, callBack, jArgs, &rval)) {
      return false;
    }

    if (!rval.toBoolean()) {
      args.rval().setBoolean(false);
      return true;
    }
  }

  args.rval().setBoolean(true);
  return true;
}

static bool array_find(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "find", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::Value callbackfn = args[0].get();

  if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
    JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "find: callback");
    return false;
  }

  JS::RootedValue selfValue(cx, jsTypeFactory(cx, self));
  JS::RootedValue callBack(cx, callbackfn);

  JS::Rooted<JS::ValueArray<3>> jArgs(cx);
  JS::RootedValue rval(cx);

  JS::RootedObject rootedThisArg(cx);
  if (args.length() > 1) {
    JS::Value thisArg = args[1].get();
    if (!thisArg.isObjectOrNull()) {
      JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_OBJORNULL, "'this' argument");
      return false;
    }

    // TODO support null, currently gets TypeError
    rootedThisArg.set(thisArg.toObjectOrNull());
    // check if callback is a PyMethod, need to make a new method bound to thisArg
    if (!makeNewPyMethod(cx, &callBack, rootedThisArg)) {
      return false;
    }
  }
  else {
    rootedThisArg.set(nullptr);
  }

  Py_ssize_t len = PyList_GET_SIZE(self);
  for (Py_ssize_t index = 0, toIndex = 0; index < len; index++) {
    JS::Value item = jsTypeFactory(cx, PyList_GetItem(self, index));
    jArgs[0].set(item);
    jArgs[1].setInt32(index);
    jArgs[2].set(selfValue);

    if (!JS_CallFunctionValue(cx, rootedThisArg, callBack, jArgs, &rval)) {
      return false;
    }

    if (rval.toBoolean()) {
      args.rval().set(item);
      return true;
    }
  }

  args.rval().setUndefined();
  return true;
}

static bool array_findIndex(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "findIndex", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::Value callbackfn = args[0].get();

  if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
    JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "findIndex: callback");
    return false;
  }

  JS::RootedValue selfValue(cx, jsTypeFactory(cx, self));
  JS::RootedValue callBack(cx, callbackfn);

  JS::Rooted<JS::ValueArray<3>> jArgs(cx);
  JS::RootedValue rval(cx);

  JS::RootedObject rootedThisArg(cx);
  if (args.length() > 1) {
    JS::Value thisArg = args[1].get();
    if (!thisArg.isObjectOrNull()) {
      JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_OBJORNULL, "'this' argument");
      return false;
    }

    // TODO support null, currently gets TypeError
    rootedThisArg.set(thisArg.toObjectOrNull());
    // check if callback is a PyMethod, need to make a new method bound to thisArg
    if (!makeNewPyMethod(cx, &callBack, rootedThisArg)) {
      return false;
    }
  }
  else {
    rootedThisArg.set(nullptr);
  }

  Py_ssize_t len = PyList_GET_SIZE(self);
  for (Py_ssize_t index = 0, toIndex = 0; index < len; index++) {
    jArgs[0].set(jsTypeFactory(cx, PyList_GetItem(self, index)));
    jArgs[1].setInt32(index);
    jArgs[2].set(selfValue);

    if (!JS_CallFunctionValue(cx, rootedThisArg, callBack, jArgs, &rval)) {
      return false;
    }

    if (rval.toBoolean()) {
      args.rval().setInt32(index);
      return true;
    }
  }

  args.rval().setInt32(-1);
  return true;
}

// private
static uint32_t FlattenIntoArray(JSContext *cx,
  JSObject *retArray, PyObject *source,
  Py_ssize_t sourceLen, uint32_t start, uint32_t depth) {

  uint32_t targetIndex = start;

  JS::RootedValue elementVal(cx);

  for (uint32_t sourceIndex = 0; sourceIndex < sourceLen; sourceIndex++) {
    if (PyObject_TypeCheck(source, &JSArrayProxyType)) {
      JS_GetElement(cx, *(((JSArrayProxy *)source)->jsArray), sourceIndex, &elementVal);
    }
    else if (PyObject_TypeCheck(source, &PyList_Type)) {
      elementVal.set(jsTypeFactory(cx, PyList_GetItem(source, sourceIndex)));
    }

    PyObject *element = pyTypeFactory(cx, elementVal);

    bool shouldFlatten;
    if (depth > 0) {
      shouldFlatten = PyObject_TypeCheck(element, &JSArrayProxyType) || PyObject_TypeCheck(element, &PyList_Type);
    } else {
      shouldFlatten = false;
    }

    if (shouldFlatten) {
      Py_ssize_t elementLen;
      if (PyObject_TypeCheck(element, &JSArrayProxyType)) {
        elementLen = JSArrayProxyMethodDefinitions::JSArrayProxy_length((JSArrayProxy *)element);
      }
      else if (PyObject_TypeCheck(element, &PyList_Type)) {
        elementLen = PyList_GET_SIZE(element);
      }

      targetIndex = FlattenIntoArray(cx,
        retArray,
        element,
        elementLen,
        targetIndex,
        depth - 1
      );
    }
    else {
      JS::RootedObject rootedRetArray(cx, retArray);

      uint32_t length;
      JS::GetArrayLength(cx, rootedRetArray, &length);
      if (targetIndex >= length) {
        JS::SetArrayLength(cx, rootedRetArray, targetIndex + 1);
      }

      JS_SetElement(cx, rootedRetArray, targetIndex, elementVal);

      targetIndex++;
    }

    Py_DECREF(element);
  }

  return targetIndex;
}

// private
static uint32_t FlattenIntoArrayWithCallBack(JSContext *cx,
  JSObject *retArray, PyObject *source,
  Py_ssize_t sourceLen, uint32_t start, uint32_t depth,
  JS::HandleValue callBack, JS::HandleObject thisArg) {

  uint32_t targetIndex = start;

  JS::RootedValue sourceValue(cx, jsTypeFactory(cx, source));
  JS::Rooted<JS::ValueArray<3>> jArgs(cx);
  JS::RootedValue elementVal(cx);
  JS::RootedValue retVal(cx);

  for (uint32_t sourceIndex = 0; sourceIndex < sourceLen; sourceIndex++) {
    if (PyObject_TypeCheck(source, &JSArrayProxyType)) {
      JS_GetElement(cx, *(((JSArrayProxy *)source)->jsArray), sourceIndex, &elementVal);
    }
    else if (PyObject_TypeCheck(source, &PyList_Type)) {
      elementVal.set(jsTypeFactory(cx, PyList_GetItem(source, sourceIndex)));
    }

    jArgs[0].set(elementVal);
    jArgs[1].setInt32(sourceIndex);
    jArgs[2].set(sourceValue);
    if (!JS_CallFunctionValue(cx, thisArg, callBack, jArgs, &retVal)) {
      return false;
    }

    PyObject *element = pyTypeFactory(cx, retVal);

    bool shouldFlatten;
    if (depth > 0) {
      shouldFlatten = PyObject_TypeCheck(element, &JSArrayProxyType) || PyObject_TypeCheck(element, &PyList_Type);
    } else {
      shouldFlatten = false;
    }

    Py_ssize_t elementLen;
    if (PyObject_TypeCheck(element, &JSArrayProxyType)) {
      elementLen = JSArrayProxyMethodDefinitions::JSArrayProxy_length((JSArrayProxy *)element);
    }
    else if (PyObject_TypeCheck(element, &PyList_Type)) {
      elementLen = PyList_GET_SIZE(element);
    }

    if (shouldFlatten) {
      targetIndex = FlattenIntoArrayWithCallBack(cx,
        retArray,
        element,
        elementLen,
        targetIndex,
        depth - 1,
        callBack,
        thisArg
      );
    }
    else {
      JS::RootedObject rootedRetArray(cx, retArray);

      uint32_t length;
      JS::GetArrayLength(cx, rootedRetArray, &length);

      if (PyObject_TypeCheck(element, &JSArrayProxyType) || PyObject_TypeCheck(element, &PyList_Type)) {
        // flatten array callBack result to depth 1
        JS::RootedValue elementIndexVal(cx);
        for (uint32_t elementIndex = 0; elementIndex < elementLen; elementIndex++, targetIndex++) {
          if (PyObject_TypeCheck(element, &JSArrayProxyType)) {
            JS_GetElement(cx, *(((JSArrayProxy *)element)->jsArray), elementIndex, &elementIndexVal);
          }
          else {
            elementIndexVal.set(jsTypeFactory(cx, PyList_GetItem(element, elementIndex)));
          }

          if (targetIndex >= length) {
            JS::SetArrayLength(cx, rootedRetArray, length = targetIndex + 1);
          }

          JS_SetElement(cx, rootedRetArray, targetIndex, elementIndexVal);
        }

        return targetIndex;
      }
      else {
        if (targetIndex >= length) {
          JS::SetArrayLength(cx, rootedRetArray, targetIndex + 1);
        }

        JS_SetElement(cx, rootedRetArray, targetIndex, retVal);

        targetIndex++;
      }
    }

    Py_DECREF(element);
  }

  return targetIndex;
}

static bool array_flat(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  Py_ssize_t sourceLen = PyList_GET_SIZE(self);

  uint32_t depthNum;
  if (args.length() > 0) {
    depthNum = args[0].get().toInt32();
  }
  else {
    depthNum = 1;
  }

  JSObject *retArray = JS::NewArrayObject(cx, sourceLen); // min end length

  FlattenIntoArray(cx, retArray, self, sourceLen, 0, depthNum);

  args.rval().setObject(*retArray);
  return true;
}

static bool array_flatMap(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.requireAtLeast(cx, "flatMap", 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  Py_ssize_t sourceLen = PyList_GET_SIZE(self);

  JS::Value callbackfn = args[0].get();

  if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
    JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "flatMap: callback");
    return false;
  }

  JS::RootedValue callBack(cx, callbackfn);

  JS::RootedObject rootedThisArg(cx);
  if (args.length() > 1) {
    JS::Value thisArg = args[1].get();
    if (!thisArg.isObjectOrNull()) {
      JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_NOT_OBJORNULL, "'this' argument");
      return false;
    }

    // TODO support null, currently gets TypeError
    rootedThisArg.set(thisArg.toObjectOrNull());
    // check if callback is a PyMethod, need to make a new method bound to thisArg
    if (!makeNewPyMethod(cx, &callBack, rootedThisArg)) {
      return false;
    }
  }
  else {
    rootedThisArg.set(nullptr);
  }

  JSObject *retArray = JS::NewArrayObject(cx, sourceLen); // min end length

  FlattenIntoArrayWithCallBack(cx, retArray, self, sourceLen, 0, 1, callBack, rootedThisArg);

  args.rval().setObject(*retArray);
  return true;
}

static bool array_join(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  Py_ssize_t selfLength = PyList_GET_SIZE(self);

  if (selfLength == 0) {
    args.rval().setString(JS_NewStringCopyZ(cx, ""));
    return true;
  }

  JS::RootedString rootedSeparator(cx);
  if (args.hasDefined(0)) {
    rootedSeparator.set(JS::ToString(cx, args[0]));
  }
  else {
    rootedSeparator.set(JS_NewStringCopyZ(cx, ","));
  }

  JSString *writer = JS_NewStringCopyZ(cx, "");
  JS::RootedString rootedWriter(cx);

  for (Py_ssize_t index = 0; index < selfLength; index++) {
    rootedWriter.set(writer);
    if (index > 0) {
      writer = JS_ConcatStrings(cx, rootedWriter, rootedSeparator);
      rootedWriter.set(writer);
    }

    JS::RootedValue element(cx, jsTypeFactory(cx, PyList_GetItem(self, index)));
    if (!element.isNullOrUndefined()) {
      JS::RootedValue rval(cx);

      JS::RootedObject retObject(cx);

      if (!JS_ValueToObject(cx, element, &retObject)) {
        return false;
      }

      if (!JS_CallFunctionName(cx, retObject, "toString", JS::HandleValueArray::empty(), &rval)) {
        return false;
      }

      JS::RootedString retString(cx, rval.toString());
      writer = JS_ConcatStrings(cx, rootedWriter, retString);
    }
  }

  args.rval().setString(writer);
  return true;
}

static bool array_toString(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_join(cx, argc, vp);
}

static bool array_toLocaleString(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  Py_ssize_t selfLength = PyList_GET_SIZE(self);

  if (selfLength == 0) {
    args.rval().setString(JS_NewStringCopyZ(cx, ""));
    return true;
  }

  JS::RootedString rootedSeparator(cx, JS_NewStringCopyZ(cx, ","));

  JSString *writer = JS_NewStringCopyZ(cx, "");
  JS::RootedString rootedWriter(cx);

  JS::HandleValueArray jArgs(args);

  for (Py_ssize_t index = 0; index < selfLength; index++) {
    rootedWriter.set(writer);
    if (index > 0) {
      writer = JS_ConcatStrings(cx, rootedWriter, rootedSeparator);
      rootedWriter.set(writer);
    }

    JS::RootedValue element(cx, jsTypeFactory(cx, PyList_GetItem(self, index)));
    if (!element.isNullOrUndefined()) {
      JS::RootedValue rval(cx);

      JS::RootedObject retObject(cx);

      if (!JS_ValueToObject(cx, element, &retObject)) {
        return false;
      }

      if (!JS_CallFunctionName(cx, retObject, "toLocaleString", jArgs, &rval)) {
        return false;
      }

      JS::RootedString retString(cx, rval.toString());
      writer = JS_ConcatStrings(cx, rootedWriter, retString);
    }
  }

  args.rval().setString(writer);
  return true;
}

static bool array_valueOf(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  // return ref to self
  args.rval().set(jsTypeFactory(cx, self));
  return true;
}


//////   Sorting


static void swapItems(PyObject *list, int i, int j) {
  if (i != j) {
    PyObject *list_i = PyList_GetItem(list, i);
    PyObject *list_j = PyList_GetItem(list, j);
    Py_INCREF(list_i);
    Py_INCREF(list_j);
    PyList_SetItem(list, i, list_j);
    PyList_SetItem(list, j, list_i);
  }
}

static int invokeCallBack(PyObject *list, int index, JS::HandleValue leftValue, JSContext *cx, JS::HandleFunction callBack) {
  JS::Rooted<JS::ValueArray<2>> jArgs(cx);

  jArgs[0].set(jsTypeFactory(cx, PyList_GetItem(list, index)));
  jArgs[1].set(leftValue);

  JS::RootedValue retVal(cx);
  if (!JS_CallFunction(cx, nullptr, callBack, jArgs, &retVal)) {
    throw "JS_CallFunction failed";
  }

  if (!retVal.isNumber()) {
    PyErr_Format(PyExc_TypeError, "incorrect compare function return type");
    return 0;
  }

  return retVal.toInt32();
}

// Adapted from Kernigan&Ritchie's C book
static void quickSort(PyObject *list, int left, int right, JSContext *cx, JS::HandleFunction callBack) {

  if (left >= right) {
    // base case
    return;
  }

  swapItems(list, left, (left + right) / 2);

  JS::RootedValue leftValue(cx, jsTypeFactory(cx, PyList_GetItem(list, left)));

  int last = left;
  for (int index = left + 1; index <= right; index++) {
    int result = invokeCallBack(list, index, leftValue, cx, callBack);
    if (PyErr_Occurred()) {
      return;
    }
    if (result < 0) {
      swapItems(list, ++last, index);
    }
  }

  swapItems(list, left, last);

  quickSort(list, left, last - 1, cx, callBack);

  quickSort(list, last + 1, right, cx, callBack);
}

// private
static bool js_sort_compare_default(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedValue leftVal(cx, args[0]);
  JS::RootedValue rightVal(cx, args[1]);

  // check for undefined
  if (leftVal.isNullOrUndefined()) {
    if (rightVal.isNullOrUndefined()) {
      args.rval().setInt32(0);
    }
    else {
      args.rval().setInt32(1);
    }
    return true;
  }
  else if (rightVal.isNullOrUndefined()) {
    args.rval().setInt32(-1);
    return true;
  }

  JS::RootedObject leftObject(cx);
  if (!JS_ValueToObject(cx, leftVal, &leftObject)) {
    return false;
  }
  JS::RootedValue leftToStringVal(cx);
  if (!JS_CallFunctionName(cx, leftObject, "toString", JS::HandleValueArray::empty(), &leftToStringVal)) {
    return false;
  }

  JS::RootedObject rightObject(cx);
  if (!JS_ValueToObject(cx, rightVal, &rightObject)) {
    return false;
  }
  JS::RootedValue rightToStringVal(cx);
  if (!JS_CallFunctionName(cx, rightObject, "toString", JS::HandleValueArray::empty(), &rightToStringVal)) {
    return false;
  }

  int32_t cmpResult;
  if (!JS_CompareStrings(cx, leftToStringVal.toString(), rightToStringVal.toString(), &cmpResult)) {
    return false;
  }

  args.rval().setInt32(cmpResult);
  return true;
}

static bool array_sort(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  Py_ssize_t len = PyList_GET_SIZE(self);

  if (len > 1) {
    if (args.length() < 1) {
      JS::RootedFunction funObj(cx, JS_NewFunction(cx, js_sort_compare_default, 2, 0, NULL));

      try {
        quickSort(self, 0, len - 1, cx, funObj);
      } catch (const char *message) {
        return false;
      }
    }
    else {
      JS::Value callbackfn = args[0].get();

      if (!callbackfn.isObject() || !JS::IsCallable(&callbackfn.toObject())) {
        JS_ReportErrorNumberASCII(cx, js::GetErrorMessage, nullptr, JSMSG_BAD_SORT_ARG);
        return false;
      }

      JS::RootedValue callBack(cx, callbackfn);
      JS::RootedFunction rootedFun(cx, JS_ValueToFunction(cx, callBack));
      try {
        quickSort(self, 0, len - 1, cx, rootedFun);
      } catch (const char *message) {
        return false;
      }
    }
  }

  if (PyErr_Occurred()) {
    return false;
  }

  // return ref to self
  args.rval().set(jsTypeFactory(cx, self));
  return true;
}



// ListIterator


#define ITEM_KIND_KEY 0
#define ITEM_KIND_VALUE 1
#define ITEM_KIND_KEY_AND_VALUE 2

enum {
  ListIteratorSlotIteratedObject,
  ListIteratorSlotNextIndex,
  ListIteratorSlotItemKind,
  ListIteratorSlotCount
};

static JSClass listIteratorClass = {"ListIterator", JSCLASS_HAS_RESERVED_SLOTS(ListIteratorSlotCount)};

static bool iterator_next(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject thisObj(cx);
  if (!args.computeThis(cx, &thisObj)) return false;

  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(thisObj, ListIteratorSlotIteratedObject);

  JS::RootedValue rootedNextIndex(cx, JS::GetReservedSlot(thisObj, ListIteratorSlotNextIndex));
  JS::RootedValue rootedItemKind(cx, JS::GetReservedSlot(thisObj, ListIteratorSlotItemKind));

  int32_t nextIndex;
  int32_t itemKind;
  if (!JS::ToInt32(cx, rootedNextIndex, &nextIndex) || !JS::ToInt32(cx, rootedItemKind, &itemKind)) return false;

  JS::RootedObject result(cx, JS_NewPlainObject(cx));

  Py_ssize_t len = PyList_GET_SIZE(self);

  if (nextIndex >= len) {
    // UnsafeSetReservedSlot(obj, ITERATOR_SLOT_TARGET, null); // TODO lose ref
    JS::RootedValue done(cx, JS::BooleanValue(true));
    if (!JS_SetProperty(cx, result, "done", done)) return false;
    args.rval().setObject(*result);
    return result;
  }

  JS::SetReservedSlot(thisObj, ListIteratorSlotNextIndex, JS::Int32Value(nextIndex + 1));

  JS::RootedValue done(cx, JS::BooleanValue(false));
  if (!JS_SetProperty(cx, result, "done", done)) return false;

  if (itemKind == ITEM_KIND_VALUE) {
    PyObject *item = PyList_GetItem(self, nextIndex);
    if (!item) {
      return false;
    }
    JS::RootedValue value(cx, jsTypeFactory(cx, item));
    if (!JS_SetProperty(cx, result, "value", value)) return false;
  }
  else if (itemKind == ITEM_KIND_KEY_AND_VALUE) {
    JS::Rooted<JS::ValueArray<2>> items(cx);

    JS::RootedValue rootedNextIndex(cx, JS::Int32Value(nextIndex));
    items[0].set(rootedNextIndex);

    PyObject *item = PyList_GetItem(self, nextIndex);
    if (!item) {
      return false;
    }
    JS::RootedValue value(cx, jsTypeFactory(cx, item));
    items[1].set(value);

    JS::RootedValue pair(cx);
    JSObject *array = JS::NewArrayObject(cx, items);
    pair.setObject(*array);
    if (!JS_SetProperty(cx, result, "value", pair)) return false;
  }
  else { // itemKind == ITEM_KIND_KEY
    JS::RootedValue value(cx, JS::Int32Value(nextIndex));
    if (!JS_SetProperty(cx, result, "value", value)) return false;
  }

  args.rval().setObject(*result);
  return true;
}

static JSFunctionSpec list_iterator_methods[] = {
  JS_FN("next", iterator_next, 0, JSPROP_ENUMERATE),
  JS_FS_END
};

static bool ListIteratorConstructor(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.isConstructing()) {
    JS_ReportErrorASCII(cx, "You must call this constructor with 'new'");
    return false;
  }

  JS::RootedObject thisObj(cx, JS_NewObjectForConstructor(cx, &listIteratorClass, args));
  if (!thisObj) {
    return false;
  }

  args.rval().setObject(*thisObj);
  return true;
}

static bool DefineListIterator(JSContext *cx, JS::HandleObject global) {
  JS::RootedObject iteratorPrototype(cx);
  if (!JS_GetClassPrototype(cx, JSProto_Iterator, &iteratorPrototype)) {
    return false;
  }

  JS::RootedObject protoObj(cx,
    JS_InitClass(cx, global,
      nullptr, iteratorPrototype,
      "ListIterator",
      ListIteratorConstructor, 0,
      nullptr, list_iterator_methods,
      nullptr, nullptr)
  );

  return protoObj; // != nullptr
}

// private util
static bool array_iterator_func(JSContext *cx, unsigned argc, JS::Value *vp, int itemKind) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::RootedObject global(cx, JS::GetNonCCWObjectGlobal(proxy));

  JS::RootedValue constructor_val(cx);
  if (!JS_GetProperty(cx, global, "ListIterator", &constructor_val)) return false;
  if (!constructor_val.isObject()) {
    if (!DefineListIterator(cx, global)) {
      return false;
    }

    if (!JS_GetProperty(cx, global, "ListIterator", &constructor_val)) return false;
    if (!constructor_val.isObject()) {
      JS_ReportErrorASCII(cx, "ListIterator is not a constructor");
      return false;
    }
  }
  JS::RootedObject constructor(cx, &constructor_val.toObject());

  JS::RootedObject obj(cx);
  if (!JS::Construct(cx, constructor_val, JS::HandleValueArray::empty(), &obj)) return false;
  if (!obj) return false;

  JS::SetReservedSlot(obj, ListIteratorSlotIteratedObject, JS::PrivateValue((void *)self));
  JS::SetReservedSlot(obj, ListIteratorSlotNextIndex, JS::Int32Value(0));
  JS::SetReservedSlot(obj, ListIteratorSlotItemKind, JS::Int32Value(itemKind));

  args.rval().setObject(*obj);
  return true;
}

static bool array_entries(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_iterator_func(cx, argc, vp, ITEM_KIND_KEY_AND_VALUE);
}

static bool array_keys(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_iterator_func(cx, argc, vp, ITEM_KIND_KEY);
}

static bool array_values(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_iterator_func(cx, argc, vp, ITEM_KIND_VALUE);
}


static JSMethodDef array_methods[] = {
  {"reverse", array_reverse, 0},
  {"pop", array_pop, 0},
  {"push", array_push, 1},
  {"shift", array_shift, 0},
  {"unshift", array_unshift, 1},
  {"concat", array_concat, 1},
  {"slice", array_slice, 2},
  {"indexOf", array_indexOf, 1},
  {"lastIndexOf", array_lastIndexOf, 1},
  {"splice", array_splice, 2},
  {"sort", array_sort, 1},
  {"fill", array_fill, 3},
  {"copyWithin", array_copyWithin, 3},
  {"includes", array_includes, 1},
  {"forEach", array_forEach, 1},
  {"map", array_map, 1},
  {"filter", array_filter, 1},
  {"reduce", array_reduce, 1},
  {"reduceRight", array_reduceRight, 1},
  {"some", array_some, 1},
  {"every", array_every, 1},
  {"find", array_find, 1},
  {"findIndex", array_findIndex, 1},
  {"flat", array_flat, 1},
  {"flatMap", array_flatMap, 1},
  {"join", array_join, 1},
  {"toString", array_toString, 0},
  {"toLocaleString", array_toLocaleString, 0},
  {"valueOf", array_valueOf, 0},
  {"entries", array_entries, 0},
  {"keys", array_keys, 0},
  {"values", array_values, 0},
  {NULL, NULL, 0}
};


bool PyListProxyHandler::getOwnPropertyDescriptor(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
) const {
  // see if we're calling a function
  if (id.isString()) {
    for (size_t index = 0;; index++) {
      bool isThatFunction;
      const char *methodName = array_methods[index].name;
      if (methodName == NULL) {   // reached end of list
        break;
      }
      else if (JS_StringEqualsAscii(cx, id.toString(), methodName, &isThatFunction) && isThatFunction) {
        JSFunction *newFunction = JS_NewFunction(cx, array_methods[index].call, array_methods[index].nargs, 0, NULL);
        if (!newFunction) return false;
        JS::RootedObject funObj(cx, JS_GetFunctionObject(newFunction));
        desc.set(mozilla::Some(
          JS::PropertyDescriptor::Data(
            JS::ObjectValue(*funObj),
            {JS::PropertyAttribute::Enumerable}
          )
        ));
        return true;
      }
    }
  }

  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  // "length" property
  bool isLengthProperty;
  if (id.isString() && JS_StringEqualsLiteral(cx, id.toString(), "length", &isLengthProperty) && isLengthProperty) {
    desc.set(mozilla::Some(
      JS::PropertyDescriptor::Data(
        JS::Int32Value(PyList_Size(self))
      )
    ));
    return true;
  }

  // "constructor" property
  bool isConstructorProperty;
  if (id.isString() && JS_StringEqualsLiteral(cx, id.toString(), "constructor", &isConstructorProperty) && isConstructorProperty) {
    JS::RootedObject rootedArrayPrototype(cx);
    if (!JS_GetClassPrototype(cx, JSProto_Array, &rootedArrayPrototype)) {
      return false;
    }

    JS::RootedValue Array_Prototype_Constructor(cx);
    if (!JS_GetProperty(cx, rootedArrayPrototype, "constructor", &Array_Prototype_Constructor)) {
      return false;
    }

    JS::RootedObject rootedArrayPrototypeConstructor(cx, Array_Prototype_Constructor.toObjectOrNull());

    desc.set(mozilla::Some(
      JS::PropertyDescriptor::Data(
        JS::ObjectValue(*rootedArrayPrototypeConstructor),
        {JS::PropertyAttribute::Enumerable}
      )
    ));
    return true;
  }

  // symbol property
  if (id.isSymbol()) {
    JS::RootedSymbol rootedSymbol(cx, id.toSymbol());

    if (JS::GetSymbolCode(rootedSymbol) == JS::SymbolCode::iterator) {
      JSFunction *newFunction = JS_NewFunction(cx, array_values, 0, 0, NULL);
      if (!newFunction) return false;
      JS::RootedObject funObj(cx, JS_GetFunctionObject(newFunction));
      desc.set(mozilla::Some(
        JS::PropertyDescriptor::Data(
          JS::ObjectValue(*funObj),
          {JS::PropertyAttribute::Enumerable}
        )
      ));
      return true;
    }
  }

  // item
  Py_ssize_t index;
  PyObject *item;
  if (idToIndex(cx, id, &index) && (item = PyList_GetItem(self, index))) {
    desc.set(mozilla::Some(
      JS::PropertyDescriptor::Data(
        jsTypeFactory(cx, item),
        {JS::PropertyAttribute::Writable, JS::PropertyAttribute::Enumerable}
      )
    ));
  } else { // item not found in list, or not an int-like property key
    desc.set(mozilla::Nothing());
  }
  return true;
}

void PyListProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {
  // We cannot call Py_DECREF here when shutting down as the thread state is gone.
  // Then, when shutting down, there is only on reference left, and we don't need
  // to free the object since the entire process memory is being released.
  if (!_Py_IsFinalizing()) {
    PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
    Py_DECREF(self);
  }
}

bool PyListProxyHandler::defineProperty(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::Handle<JS::PropertyDescriptor> desc, JS::ObjectOpResult &result
) const {
  Py_ssize_t index;
  if (!idToIndex(cx, id, &index)) { // not an int-like property key
    return result.failBadIndex();
  }

  if (desc.isAccessorDescriptor()) { // containing getter/setter
    return result.failNotDataDescriptor();
  }
  if (!desc.hasValue()) {
    return result.failInvalidDescriptor();
  }

  JS::RootedValue itemV(cx, desc.value());
  PyObject *item = pyTypeFactory(cx, itemV);
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  if (PyList_SetItem(self, index, item) < 0) {
    // we are out-of-bounds and need to expand
    Py_ssize_t len = PyList_GET_SIZE(self);
    // fill the space until the inserted index
    for (Py_ssize_t i = len; i < index; i++) {
      PyList_Append(self, Py_None);
    }

    PyList_Append(self, item);

    // clear pending exception
    PyErr_Clear();
  }

  return result.succeed();
}

bool PyListProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const {
  // Modified from https://hg.mozilla.org/releases/mozilla-esr102/file/3b574e1/dom/base/RemoteOuterWindowProxy.cpp#l137
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  int32_t length = PyList_Size(self);
  if (!props.reserve(length + 1)) {
    return false;
  }
  // item indexes
  for (int32_t i = 0; i < length; ++i) {
    props.infallibleAppend(JS::PropertyKey::Int(i));
  }
  // the "length" property
  props.infallibleAppend(JS::PropertyKey::NonIntAtom(JS_AtomizeString(cx, "length")));
  return true;
}

bool PyListProxyHandler::delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id, JS::ObjectOpResult &result) const {
  Py_ssize_t index;
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  if (!idToIndex(cx, id, &index)) {
    return result.failBadIndex();
  }

  // Set to undefined instead of actually deleting it
  if (PyList_SetItem(self, index, Py_None) < 0) {
    return result.failCantDelete();
  }
  return result.succeed();
}

bool PyListProxyHandler::isArray(JSContext *cx, JS::HandleObject proxy, JS::IsArrayAnswer *answer) const {
  *answer = JS::IsArrayAnswer::Array;
  return true;
}

bool PyListProxyHandler::getBuiltinClass(JSContext *cx, JS::HandleObject proxy, js::ESClass *cls) const {
  *cls = js::ESClass::Array;
  return true;
}