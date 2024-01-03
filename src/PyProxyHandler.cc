/**
 * @file PyProxyHandler.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS proxy objects. Used by DictType for object coercion TODO
 * @version 0.1
 * @date 2023-04-20
 *
 * Copyright (c) 2023-2024 Distributive Corp.
 *
 */

#include "include/PyProxyHandler.hh"

#include "include/jsTypeFactory.hh"
#include "include/pyTypeFactory.hh"
#include "include/StrType.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Conversions.h>
#include <js/Proxy.h>
#include <js/Symbol.h>

#include <Python.h>

PyObject *idToKey(JSContext *cx, JS::HandleId id) {
  JS::RootedValue idv(cx, js::IdToValue(id));
  JS::RootedString idStr(cx);
  if (!id.isSymbol()) { // `JS::ToString` returns `nullptr` for JS symbols
    idStr = JS::ToString(cx, idv);
  } else {
    // TODO (Tom Tang): Revisit this once we have Symbol coercion support
    // FIXME (Tom Tang): key collision for symbols without a description string, or pure strings look like "Symbol(xxx)"
    idStr = JS_ValueToSource(cx, idv);
  }

  // We convert all types of property keys to string
  auto chars = JS_EncodeStringToUTF8(cx, idStr);
  return PyUnicode_FromString(chars.get());
}

bool idToIndex(JSContext *cx, JS::HandleId id, Py_ssize_t *index) {
  if (id.isInt()) { // int-like strings have already been automatically converted to ints
    *index = id.toInt();
    return true;
  } else {
    return false; // fail
  }
}

const char PyProxyHandler::family = 0;

bool PyProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const {
  PyObject *keys = PyDict_Keys(pyObject);
  size_t length = PyList_Size(keys);
  if (!props.reserve(length)) {
    return false; // out of memory
  }

  for (size_t i = 0; i < length; i++) {
    PyObject *key = PyList_GetItem(keys, i);
    JS::RootedId jsId(cx);
    if (!keyToId(key, &jsId)) {
      // TODO (Caleb Aikens): raise exception here
      return false; // key is not a str or int
    }
    props.infallibleAppend(jsId);
  }
  return true;
}

bool PyProxyHandler::delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::ObjectOpResult &result) const {
  PyObject *attrName = idToKey(cx, id);
  if (PyDict_DelItem(pyObject, attrName) < 0) {
    return result.failCantDelete(); // raises JS exception
  }
  return result.succeed();
}

bool PyProxyHandler::has(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
  return hasOwn(cx, proxy, id, bp);
}

bool PyProxyHandler::get(JSContext *cx, JS::HandleObject proxy,
  JS::HandleValue receiver, JS::HandleId id,
  JS::MutableHandleValue vp) const {
  PyObject *attrName = idToKey(cx, id);
  PyObject *p = PyDict_GetItemWithError(pyObject, attrName);
  if (!p) { // NULL if the key is not present
    vp.setUndefined(); // JS objects return undefined for nonpresent keys
  } else {
    vp.set(jsTypeFactory(cx, p));
  }
  return true;
}

bool PyProxyHandler::getOwnPropertyDescriptor(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
) const {
  PyObject *attrName = idToKey(cx, id);
  PyObject *item = PyDict_GetItemWithError(pyObject, attrName);
  if (!item) { // NULL if the key is not present
    desc.set(mozilla::Nothing()); // JS objects return undefined for nonpresent keys
  } else {
    desc.set(mozilla::Some(
      JS::PropertyDescriptor::Data(
        jsTypeFactory(cx, item),
        {JS::PropertyAttribute::Writable, JS::PropertyAttribute::Enumerable}
      )
    ));
  }
  return true;
}

bool PyProxyHandler::set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::HandleValue v, JS::HandleValue receiver,
  JS::ObjectOpResult &result) const {
  JS::RootedValue *rootedV = new JS::RootedValue(cx, v);
  PyObject *attrName = idToKey(cx, id);
  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  if (PyDict_SetItem(pyObject, attrName, pyTypeFactory(cx, global, rootedV)->getPyObject())) {
    return result.failCantSetInterposed(); // raises JS exception
  }
  return result.succeed();
}

bool PyProxyHandler::enumerate(JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

bool PyProxyHandler::hasOwn(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  bool *bp) const {
  PyObject *attrName = idToKey(cx, id);
  *bp = PyDict_Contains(pyObject, attrName) == 1;
  return true;
}

bool PyProxyHandler::getOwnEnumerablePropertyKeys(
  JSContext *cx, JS::HandleObject proxy,
  JS::MutableHandleIdVector props) const {
  return this->ownPropertyKeys(cx, proxy, props);
}

// @TODO (Caleb Aikens) implement this
void PyProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {}

bool PyProxyHandler::defineProperty(JSContext *cx, JS::HandleObject proxy,
  JS::HandleId id,
  JS::Handle<JS::PropertyDescriptor> desc,
  JS::ObjectOpResult &result) const {
  // Block direct `Object.defineProperty` since we already have the `set` method
  return result.failInvalidDescriptor();
}

bool PyBaseProxyHandler::getPrototypeIfOrdinary(JSContext *cx, JS::HandleObject proxy,
  bool *isOrdinary,
  JS::MutableHandleObject protop) const {
  // We don't have a custom [[GetPrototypeOf]]
  *isOrdinary = true;
  protop.set(js::GetStaticPrototype(proxy));
  return true;
}

bool PyBaseProxyHandler::preventExtensions(JSContext *cx, JS::HandleObject proxy,
  JS::ObjectOpResult &result) const {
  result.succeed();
  return true;
}

bool PyBaseProxyHandler::isExtensible(JSContext *cx, JS::HandleObject proxy,
  bool *extensible) const {
  *extensible = false;
  return true;
}



// PyList ----------------------------------------------------------------------
const char PyListProxyHandler::family = 0;

static bool array_reverse(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  if (PyList_GET_SIZE(self) > 1) {
    PyList_Reverse(self);
  }

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
  return true;
}

static bool array_push(JSContext *cx, unsigned argc, JS::Value *vp) { // surely the function name is in there...review JSAPI examples
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  unsigned numArgs = args.length();
  for (unsigned index = 0; index < numArgs; index++) {
    JS::RootedValue *elementVal = new JS::RootedValue(cx);
    elementVal->set(args[index].get());
    PyList_Append(self, pyTypeFactory(cx, global, elementVal)->getPyObject());
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
  PySequence_DelItem(self, 0);

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

  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  for (int index = args.length() - 1; index >= 0; index--) {
    JS::RootedValue *elementVal = new JS::RootedValue(cx);
    elementVal->set(args[index].get());
    PyList_Insert(self, 0, pyTypeFactory(cx, global, elementVal)->getPyObject());
  }

  args.rval().setInt32(PyList_GET_SIZE(self));
  return true;
}

static bool array_concat(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::RootedObject selfCopy(cx, &jsTypeFactoryCopy(cx, self).toObject());

  JS::HandleValueArray jArgs(args);

  JS::RootedValue jRet(cx);
  if (!JS_CallFunctionName(cx, selfCopy, "concat", jArgs, &jRet)) {
    return false;
  }

  args.rval().setObject(jRet.toObject());
  return true;
}

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

  args.rval().set(jsTypeFactory(cx, result));
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

  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  JS::RootedValue *elementVal = new JS::RootedValue(cx, args[0].get());
  PyObject *result = PyObject_CallMethod(self, "index", "Oi", pyTypeFactory(cx, global, elementVal)->getPyObject(), start);

  if (!result) {
    PyErr_Clear();
    args.rval().setInt32(-1);
    return true;
  }

  args.rval().set(jsTypeFactory(cx, result));
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

  JS::RootedObject selfCopy(cx, &jsTypeFactoryCopy(cx, self).toObject());

  JS::HandleValueArray jArgs(args);

  JS::RootedValue jRet(cx);
  if (!JS_CallFunctionName(cx, selfCopy, "lastIndexOf", jArgs, &jRet)) {
    return false;
  }

  args.rval().setInt32(jRet.toInt32());
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

  // build list for SetSlice call
  PyObject *inserted = PyList_New(insertCount);

  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  for (int index = 0; index < insertCount; index++) {
    JS::RootedValue *elementVal = new JS::RootedValue(cx, args[index + 2].get());
    PyList_SetItem(inserted, index, pyTypeFactory(cx, global, elementVal)->getPyObject());
  }

  PyList_SetSlice(self, actualStart, actualStart + actualDeleteCount, inserted);

  args.rval().set(jsTypeFactory(cx, deleted));
  return true;
}

static bool array_sort(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  uint64_t selfLength = (uint64_t)PyList_GET_SIZE(self);

  if (selfLength > 0) {
    if (args.length() < 1) {
      PyList_Sort(self);
    }
    else {
      JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));

      PyObject *pyFunc = pyTypeFactory(cx, global, new JS::RootedValue(cx, args[0].get()))->getPyObject();
      // check if JS or Python function
      if (PyFunction_Check(pyFunc)) {
        // it's a user-defined python function, more than 1-arg will get TypeError
        PyObject *callable = PyObject_GetAttrString(self, "sort");
        if (callable == NULL) {
          return false;
        }
        PyObject *result = PyObject_Call(callable, PyTuple_New(0), Py_BuildValue("{s:O}", "key", pyFunc));
        if (!result) {
          return false;
        }
      } else {
        // it's either a JS function or a builtin python func
        int flags = PyCFunction_GetFlags(pyFunc);

        if (flags & METH_VARARGS && !(flags & METH_KEYWORDS)) {
          // it's a JS func

          // We don't want to put in all the sort code so we'll tolerate the following slight O(n) inefficiency

          // copy to JS for sorting
          JS::RootedObject selfCopy(cx, &jsTypeFactoryCopy(cx, self).toObject());

          // sort
          JS::RootedValue jReturnedArray(cx);
          JS::HandleValueArray jArgs(args);
          if (!JS_CallFunctionName(cx, selfCopy, "sort", jArgs, &jReturnedArray)) {
            return false;
          }

          // copy back into Python self
          for (int index = 0; index < selfLength; index++) {
            JS::RootedValue *elementVal = new JS::RootedValue(cx);
            JS_GetElement(cx, selfCopy, index, elementVal);
            PyList_SetItem(self, index, pyTypeFactory(cx, global, elementVal)->getPyObject());
          }
        } else {
          // it's a built-in python function, more than 1-arg will get TypeError
          PyObject *callable = PyObject_GetAttrString(self, "sort");
          if (callable == NULL) {
            return false;
          }
          PyObject *result = PyObject_Call(callable, PyTuple_New(0), Py_BuildValue("{s:O}", "key", pyFunc));
          if (!result) {
            return false;
          }
        }
      }
    }
  }

  // return ref to self
  args.rval().set(jsTypeFactory(cx, self));
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

  JS::RootedValue *fillValue = new JS::RootedValue(cx, args[0].get());

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

  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  for (int index = actualStart; index < actualEnd; index++) {
    PyList_SetItem(self, index, pyTypeFactory(cx, global, fillValue)->getPyObject());
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

  uint64_t selfLength = (uint64_t)PyList_GET_SIZE(self);

  unsigned int argsLength = args.length();

  int64_t relativeTarget;
  if (argsLength > 0) {
    if (!JS::ToInt64(cx, args.get(0), &relativeTarget)) {
      return false;
    }
  } else {
    relativeTarget = 0;
  }

  uint64_t actualTarget;
  if (relativeTarget < 0) {
    actualTarget = uint64_t(std::max(double(selfLength) + relativeTarget, 0.0));
  } else {
    actualTarget = uint64_t(std::min(double(relativeTarget), double(selfLength)));
  }

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

  int64_t count = int64_t(std::min(actualEnd - actualStart, selfLength - actualTarget));

  if (actualStart < actualTarget && actualTarget < actualStart + count) {
    actualStart = actualStart + count - 1;
    actualTarget = actualTarget + count - 1;

    while (count > 0) {
      PyObject *itemStart = PyList_GetItem(self, actualStart);
      PyList_SetItem(self, actualTarget, itemStart);

      actualStart--;
      actualTarget--;
      count--;
    }
  } else {
    while (count > 0) {
      PyObject *itemStart = PyList_GetItem(self, actualStart);
      PyList_SetItem(self, actualTarget, itemStart);

      actualStart++;
      actualTarget++;
      count--;
    }
  }

  // return ref to self
  args.rval().set(jsTypeFactory(cx, self));
  return true;
}

// private util
static bool array_copy_func(JSContext *cx, unsigned argc, JS::Value *vp, const char *fName, bool checkRequireAtLeastOne = true) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (checkRequireAtLeastOne && !args.requireAtLeast(cx, fName, 1)) {
    return false;
  }

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::RootedObject selfCopy(cx, &jsTypeFactoryCopy(cx, self).toObject());

  JS::HandleValueArray jArgs(args);
  JS::RootedValue rval(cx);

  if (!JS_CallFunctionName(cx, selfCopy, fName, jArgs, &rval)) {
    return false;
  }

  args.rval().set(rval);

  return true;
}

static bool array_includes(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "includes");
}

static bool array_forEach(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "forEach");
}

static bool array_map(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "map");
}

static bool array_filter(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "filter");
}

static bool array_reduce(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "reduce");
}

static bool array_reduceRight(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "reduceRight");
}

static bool array_some(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "some");
}

static bool array_every(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "every");
}

static bool array_find(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "find");
}

static bool array_findIndex(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "findIndex");
}

static bool array_flat(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "flat", false);
}

static bool array_flatMap(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "flatMap");
}

static bool array_join(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "join", false);
}

static bool array_toString(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "toString", false);
}

static bool array_toLocaleString(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "toLocaleString", false);
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

static bool array_entries(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "entries", false);
}

static bool array_keys(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "keys", false);
}

static bool array_values(JSContext *cx, unsigned argc, JS::Value *vp) {
  return array_copy_func(cx, argc, vp, "values", false);
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

  // "length" property
  bool isLengthProperty;
  if (id.isString() && JS_StringEqualsLiteral(cx, id.toString(), "length", &isLengthProperty) && isLengthProperty) {
    desc.set(mozilla::Some(
      JS::PropertyDescriptor::Data(
        JS::Int32Value(PyList_Size(pyObject))
      )
    ));
    return true;
  }

  // "constructor" property
  bool isConstructorProperty;
  if (id.isString() && JS_StringEqualsLiteral(cx, id.toString(), "constructor", &isConstructorProperty) && isConstructorProperty) {
    JS::RootedObject global(cx, JS::GetNonCCWObjectGlobal(proxy));

    JS::RootedValue Array(cx);
    if (!JS_GetProperty(cx, global, "Array", &Array)) {
      return false;
    }

    JS::RootedObject rootedArray(cx, Array.toObjectOrNull());

    JS::RootedValue Array_Prototype(cx);
    if (!JS_GetProperty(cx, rootedArray, "prototype", &Array_Prototype)) {
      return false;
    }

    JS::RootedObject rootedArrayPrototype(cx, Array_Prototype.toObjectOrNull());

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

  //  item
  Py_ssize_t index;
  PyObject *item;
  if (idToIndex(cx, id, &index) && (item = PyList_GetItem(pyObject, index))) {
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
  // TODO
  // PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  // printf("finalize self=%p\n", self);
  // Py_DECREF(self);
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

  // FIXME (Tom Tang): memory leak
  JS::RootedObject *global = new JS::RootedObject(cx, JS::GetNonCCWObjectGlobal(proxy));
  JS::RootedValue *itemV = new JS::RootedValue(cx, desc.value());
  PyObject *item = pyTypeFactory(cx, global, itemV)->getPyObject();
  if (PyList_SetItem(pyObject, index, item) < 0) {
    return result.failBadIndex();
  }
  return result.succeed();
}

bool PyListProxyHandler::ownPropertyKeys(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const {
  // Modified from https://hg.mozilla.org/releases/mozilla-esr102/file/3b574e1/dom/base/RemoteOuterWindowProxy.cpp#l137
  int32_t length = PyList_Size(pyObject);
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
  if (!idToIndex(cx, id, &index)) {
    return result.failBadIndex(); // report failure
  }

  // Set to undefined instead of actually deleting it
  if (PyList_SetItem(pyObject, index, Py_None) < 0) {
    return result.failCantDelete(); // report failure
  }
  return result.succeed(); // report success
}

bool PyListProxyHandler::isArray(JSContext *cx, JS::HandleObject proxy, JS::IsArrayAnswer *answer) const {
  *answer = JS::IsArrayAnswer::Array;
  return true;
}

bool PyListProxyHandler::getBuiltinClass(JSContext *cx, JS::Handle<JSObject *> obj, js::ESClass *cls) const {
  *cls = js::ESClass::Array;
  return true;
}

const char *PyListProxyHandler::className(JSContext *cx, JS::HandleObject proxy) const {
  // TODO untested
  return "Array";
}