/**
 * @file PyBytesProxyHandler.cc
 * @author Philippe Laporte (philippe@distributive.network)
 * @brief Struct for creating JS proxy objects for immutable bytes objects
 * @date 2024-07-23
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */


#include "include/PyBytesProxyHandler.hh"

#include <jsapi.h>
#include <js/ArrayBuffer.h>

#include <Python.h>


const char PyBytesProxyHandler::family = 0;


static bool bytes_valueOf(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject proxy(cx, JS::ToObject(cx, args.thisv()));
  if (!proxy) {
    return false;
  }

  JS::PersistentRootedObject* arrayBuffer = JS::GetMaybePtrFromReservedSlot<JS::PersistentRootedObject>(proxy, OtherSlot);
  JS::RootedObject rootedArrayBuffer(cx, arrayBuffer->get());

  auto byteLength = JS::GetArrayBufferByteLength(rootedArrayBuffer);

  bool isSharedMemory; 
  JS::AutoCheckCannotGC autoNoGC(cx);
  uint8_t *data = JS::GetArrayBufferData(rootedArrayBuffer, &isSharedMemory, autoNoGC);

  std::string valueOfString;

  for (Py_ssize_t index = 0; index < byteLength; index++) {
    if (index > 0) {
      valueOfString += ",";
    }
    valueOfString += data[index];
  }

  args.rval().setString(JS_NewStringCopyZ(cx, valueOfString.c_str()));
  return true;
}

static bool bytes_toString(JSContext *cx, unsigned argc, JS::Value *vp) {
  return bytes_valueOf(cx, argc, vp);
}

JSMethodDef PyBytesProxyHandler::bytes_methods[] = {
  {"toString", bytes_toString, 0},
  {"valueOf", bytes_valueOf, 0},
  {NULL, NULL, 0}
};


bool PyBytesProxyHandler::set(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::HandleValue v, JS::HandleValue receiver,
  JS::ObjectOpResult &result) const {

  // block all modifications  
  
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);

  PyErr_Format(PyExc_TypeError,
               "'%.100s' object has only read-only attributes",
               Py_TYPE(self)->tp_name);

  return result.failReadOnly();
}

bool PyBytesProxyHandler::getOwnPropertyDescriptor(
  JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
  JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc
) const {
  // see if we're calling a function
  if (id.isString()) {
    for (size_t index = 0;; index++) {
      bool isThatFunction;
      const char *methodName = bytes_methods[index].name;
      if (methodName == NULL) {
        break;
      }
      else if (JS_StringEqualsAscii(cx, id.toString(), methodName, &isThatFunction) && isThatFunction) {
        JSFunction *newFunction = JS_NewFunction(cx, bytes_methods[index].call, bytes_methods[index].nargs, 0, NULL);
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

  if (id.isString()) {
    bool isProperty;

    // "length" and byteLength properties have the same value
    if (((JS_StringEqualsLiteral(cx, id.toString(), "length", &isProperty) && isProperty) || (JS_StringEqualsLiteral(cx, id.toString(), "byteLength", &isProperty) && isProperty))) {
      JS::PersistentRootedObject* arrayBuffer = JS::GetMaybePtrFromReservedSlot<JS::PersistentRootedObject>(proxy, OtherSlot);

      JS::RootedObject rootedArrayBuffer(cx, arrayBuffer->get());

      auto byteLength = JS::GetArrayBufferByteLength(rootedArrayBuffer);

      desc.set(mozilla::Some(
        JS::PropertyDescriptor::Data(
          JS::Int32Value(byteLength)
        )
      ));
      return true;
    }

    // buffer property
    if (JS_StringEqualsLiteral(cx, id.toString(), "buffer", &isProperty) && isProperty) {
      JS::PersistentRootedObject* arrayBuffer = JS::GetMaybePtrFromReservedSlot<JS::PersistentRootedObject>(proxy, OtherSlot);

      desc.set(mozilla::Some(
        JS::PropertyDescriptor::Data(
          JS::ObjectValue(*(arrayBuffer->get()))
        )
      ));
      return true;
    }

    // BYTES_PER_ELEMENT property
    if (JS_StringEqualsLiteral(cx, id.toString(), "BYTES_PER_ELEMENT", &isProperty) && isProperty) {
      desc.set(mozilla::Some(
        JS::PropertyDescriptor::Data(
          JS::Int32Value(1)
        )
      ));
      return true;
    }

    // byteOffset property
    if (JS_StringEqualsLiteral(cx, id.toString(), "byteOffset", &isProperty) && isProperty) {
      desc.set(mozilla::Some(
        JS::PropertyDescriptor::Data(
          JS::Int32Value(0)
        )
      ));
      return true;
    }

    // "constructor" property
    if (JS_StringEqualsLiteral(cx, id.toString(), "constructor", &isProperty) && isProperty) {
      JS::RootedObject global(cx, JS::GetNonCCWObjectGlobal(proxy));

      JS::RootedObject uint8ArrayPrototype(cx);
      if (!JS_GetClassPrototype(cx, JSProto_Uint8Array, &uint8ArrayPrototype)) {
        return false;
      }

      JS::RootedValue Uint8Array_Prototype_Constructor(cx);
      if (!JS_GetProperty(cx, uint8ArrayPrototype, "constructor", &Uint8Array_Prototype_Constructor)) {
        return false;
      }

      JS::RootedObject rootedUint8ArrayPrototypeConstructor(cx, Uint8Array_Prototype_Constructor.toObjectOrNull());

      desc.set(mozilla::Some(
        JS::PropertyDescriptor::Data(
          JS::ObjectValue(*rootedUint8ArrayPrototypeConstructor),
          {JS::PropertyAttribute::Enumerable}
        )
      ));

      return true;
    }
  }

  if (id.isSymbol()) {
    return true; // needed for console.log
  }

  // item
  Py_ssize_t index;
  if (idToIndex(cx, id, &index)) {
    JS::PersistentRootedObject* arrayBuffer = JS::GetMaybePtrFromReservedSlot<JS::PersistentRootedObject>(proxy, OtherSlot);
    JS::RootedObject rootedArrayBuffer(cx, arrayBuffer->get());

    bool isSharedMemory; 
    JS::AutoCheckCannotGC autoNoGC(cx);
    uint8_t *data = JS::GetArrayBufferData(rootedArrayBuffer, &isSharedMemory, autoNoGC);

    desc.set(mozilla::Some(
      JS::PropertyDescriptor::Data(
        JS::Int32Value(data[index])
      )
    ));

    return true;
  } 

  PyObject *attrName = idToKey(cx, id);
  PyObject *self = JS::GetMaybePtrFromReservedSlot<PyObject>(proxy, PyObjectSlot);
  PyObject *item = PyObject_GetAttr(self, attrName);

  return handleGetOwnPropertyDescriptor(cx, id, desc, item);
}

void PyBytesProxyHandler::finalize(JS::GCContext *gcx, JSObject *proxy) const {
  PyObjectProxyHandler::finalize(gcx, proxy);

  JS::PersistentRootedObject *arrayBuffer = JS::GetMaybePtrFromReservedSlot<JS::PersistentRootedObject>(proxy, OtherSlot);
  delete arrayBuffer;
}