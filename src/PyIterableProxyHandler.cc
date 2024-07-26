/**
 * @file PyIterableProxyHandler.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS proxy objects for Iterables
 * @date 2024-04-08
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */


#include "include/PyIterableProxyHandler.hh"

#include "include/jsTypeFactory.hh"

#include <jsapi.h>

#include <Python.h>



const char PyIterableProxyHandler::family = 0;


static bool iter_next(JSContext *cx, JS::CallArgs args, PyObject *it) {
  JS::RootedObject result(cx, JS_NewPlainObject(cx));

  PyObject *(*iternext)(PyObject *) = *Py_TYPE(it)->tp_iternext;

  PyObject *item = iternext(it);

  if (item == NULL) {
    if (PyErr_Occurred()) {
      if (PyErr_ExceptionMatches(PyExc_StopIteration) ||
          PyErr_ExceptionMatches(PyExc_SystemError)) {      // TODO this handles a result like   SystemError: Objects/dictobject.c:1778: bad argument to internal function. Why are we getting that?
        PyErr_Clear();
      }
      else {
        return false;
      }
    }

    JS::RootedValue done(cx, JS::BooleanValue(true));
    if (!JS_SetProperty(cx, result, "done", done)) return false;
    args.rval().setObject(*result);
    return result;
  }

  JS::RootedValue done(cx, JS::BooleanValue(false));
  if (!JS_SetProperty(cx, result, "done", done)) return false;

  JS::RootedValue value(cx, jsTypeFactory(cx, item));
  if (!JS_SetProperty(cx, result, "value", value)) return false;

  args.rval().setObject(*result);
  return true;
}

static bool iterable_next(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject thisObj(cx);
  if (!args.computeThis(cx, &thisObj)) return false;

  PyObject *it = JS::GetMaybePtrFromReservedSlot<PyObject>(thisObj, PyObjectSlot);

  return iter_next(cx, args, it);
}

static bool toPrimitive(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }

  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  _PyUnicodeWriter writer;

  _PyUnicodeWriter_Init(&writer);
  
  PyObject *s = PyObject_Repr(self);

  if (s == nullptr) {
    args.rval().setString(JS_NewStringCopyZ(cx, "<cannot repr type>"));
    return true;
  }

  int res = _PyUnicodeWriter_WriteStr(&writer, s);
  Py_DECREF(s);

  if (res < 0) {
    args.rval().setString(JS_NewStringCopyZ(cx, "<cannot repr type>"));
    return true;
  }

  PyObject* repr = _PyUnicodeWriter_Finish(&writer);
 
  args.rval().set(jsTypeFactory(cx, repr));
  return true;
}

static bool iterable_valueOf(JSContext *cx, unsigned argc, JS::Value *vp) {
  return toPrimitive(cx, argc, vp);
}

static JSMethodDef iterable_methods[] = {
  {"next", iterable_next, 0},
  {"valueOf", iterable_valueOf, 0},
  {NULL, NULL, 0}
};


// IterableIterator

enum {
  IterableIteratorSlotIterableObject,
  IterableIteratorSlotCount
};

static JSClass iterableIteratorClass = {"IterableIterator", JSCLASS_HAS_RESERVED_SLOTS(IterableIteratorSlotCount)};

static bool iterator_next(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject thisObj(cx);
  if (!args.computeThis(cx, &thisObj)) return false;

  PyObject *it = JS::GetMaybePtrFromReservedSlot<PyObject>(thisObj, IterableIteratorSlotIterableObject);

  return iter_next(cx, args, it);
}

static JSFunctionSpec iterable_iterator_methods[] = {
  JS_FN("next", iterator_next, 0, JSPROP_ENUMERATE),
  JS_FS_END
};

static bool IterableIteratorConstructor(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (!args.isConstructing()) {
    JS_ReportErrorASCII(cx, "You must call this constructor with 'new'");
    return false;
  }

  JS::RootedObject thisObj(cx, JS_NewObjectForConstructor(cx, &iterableIteratorClass, args));
  if (!thisObj) {
    return false;
  }

  args.rval().setObject(*thisObj);
  return true;
}

static bool DefineIterableIterator(JSContext *cx, JS::HandleObject global) {
  JS::RootedObject iteratorPrototype(cx);
  if (!JS_GetClassPrototype(cx, JSProto_Iterator, &iteratorPrototype)) {
    return false;
  }

  JS::RootedObject protoObj(cx,
    JS_InitClass(cx, global,
      nullptr, iteratorPrototype,
      "IterableIterator",
      IterableIteratorConstructor, 0,
      nullptr, iterable_iterator_methods,
      nullptr, nullptr)
  );

  return protoObj; // != nullptr
}

static bool iterable_values(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }

  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  JS::RootedObject global(cx, JS::GetNonCCWObjectGlobal(proxy));

  JS::RootedValue constructor_val(cx);
  if (!JS_GetProperty(cx, global, "IterableIterator", &constructor_val)) return false;
  if (!constructor_val.isObject()) {
    if (!DefineIterableIterator(cx, global)) {
      return false;
    }

    if (!JS_GetProperty(cx, global, "IterableIterator", &constructor_val)) return false;
    if (!constructor_val.isObject()) {
      JS_ReportErrorASCII(cx, "IterableIterator is not a constructor");
      return false;
    }
  }
  JS::RootedObject constructor(cx, &constructor_val.toObject());

  JS::RootedObject obj(cx);
  if (!JS::Construct(cx, constructor_val, JS::HandleValueArray::empty(), &obj)) return false;
  if (!obj) return false;

  JS::SetReservedSlot(obj, IterableIteratorSlotIterableObject, JS::PrivateValue((void *)self));

  args.rval().setObject(*obj);
  return true;
}

bool PyIterableProxyHandler::getOwnPropertyDescriptor(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
) const {
  // see if we're calling a function
  if (id.isString()) {
    for (size_t index = 0;; index++) {
      bool isThatFunction;
      const char *methodName = iterable_methods[index].name;
      if (methodName == NULL) {
        break;
      }
      else if (JS_StringEqualsAscii(cx, id.toString(), methodName, &isThatFunction) && isThatFunction) {
        JSFunction *newFunction = JS_NewFunction(cx, iterable_methods[index].call, iterable_methods[index].nargs, 0, NULL);
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

  // "constructor" property
  bool isConstructorProperty;
  if (id.isString() && JS_StringEqualsLiteral(cx, id.toString(), "constructor", &isConstructorProperty) && isConstructorProperty) {
    JS::RootedObject rootedObjectPrototype(cx);
    if (!JS_GetClassPrototype(cx, JSProto_Object, &rootedObjectPrototype)) {
      return false;
    }

    JS::RootedValue Object_Prototype_Constructor(cx);
    if (!JS_GetProperty(cx, rootedObjectPrototype, "constructor", &Object_Prototype_Constructor)) {
      return false;
    }

    JS::RootedObject rootedObjectPrototypeConstructor(cx, Object_Prototype_Constructor.toObjectOrNull());

    desc.set(mozilla::Some(
      JS::PropertyDescriptor::Data(
        JS::ObjectValue(*rootedObjectPrototypeConstructor),
        {JS::PropertyAttribute::Enumerable}
      )
    ));
    return true;
  }

  // symbol property
  if (id.isSymbol()) {
    JS::RootedSymbol rootedSymbol(cx, id.toSymbol());
    JS::SymbolCode symbolCode = JS::GetSymbolCode(rootedSymbol); 

    if (symbolCode == JS::SymbolCode::iterator) {
      JSFunction *newFunction = JS_NewFunction(cx, iterable_values, 0, 0, NULL);
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
    else if (symbolCode == JS::SymbolCode::toPrimitive) {
      JSFunction *newFunction = JS_NewFunction(cx, toPrimitive, 0, 0, nullptr);
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

  PyObject *attrName = idToKey(cx, id);
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  PyObject *item = PyObject_GetAttr(self, attrName);

  return handleGetOwnPropertyDescriptor(cx, id, desc, item);
}