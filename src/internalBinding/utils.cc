/**
 * @file utils.cc
 * @author Tom Tang (xmader@distributive.network)
 * @brief Implement functions in `internalBinding("utils")`
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 */

#include "include/internalBinding.hh"

#include <jsapi.h>
#include <js/Array.h>
#include <js/ArrayBufferMaybeShared.h>
#include <js/Conversions.h>
#include <js/Promise.h>
#include <js/Proxy.h>
#include <js/RegExp.h>
#include <js/experimental/TypedData.h>

/**
 * See function declarations in python/pythonmonkey/builtin_modules/internal-binding.d.ts :
 *    `declare function internalBinding(namespace: "utils")`
 */

static bool defineGlobal(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::HandleValue nameVal = args.get(0);
  JS::HandleValue value = args.get(1);
  args.rval().setUndefined();

  JS::RootedObject globalObj(cx, JS::CurrentGlobalOrNull(cx));
  JS::RootedId name(cx);

  return JS_ValueToId(cx, nameVal, &name) &&
         JS_DefinePropertyById(cx, globalObj, name, value, 0); // Object.defineProperty(obj, name, { value })
}

static bool isAnyArrayBuffer(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JSObject *obj = JS::ToObject(cx, args.get(0));
  args.rval().setBoolean(JS::IsArrayBufferObjectMaybeShared(obj)); // ArrayBuffer or SharedArrayBuffer
  return true;
}

static bool isPromise(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::HandleValue objVal = args.get(0);
  JS::RootedObject obj(cx, JS::ToObject(cx, objVal));
  args.rval().setBoolean(JS::IsPromiseObject(obj));
  return true;
}

static bool isRegExp(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::HandleValue objVal = args.get(0);

  bool objIsRegExp;
  JS::ObjectIsRegExp(cx, JS::RootedObject(cx, JS::ToObject(cx, objVal)), &objIsRegExp);

  args.rval().setBoolean(objIsRegExp);
  return true;
}

static bool isTypedArray(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::HandleValue objVal = args.get(0);
  args.rval().setBoolean(JS_IsTypedArrayObject(JS::ToObject(cx, objVal)));
  return true;
}

static bool getPromiseDetails(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject promise(cx, JS::ToObject(cx, args.get(0)));
  JS::RootedValueArray<2> resArr(cx);

  JS::PromiseState state = JS::GetPromiseState(promise);
  resArr[0].set(JS::NumberValue((uint32_t)state));
  if (state != JS::PromiseState::Pending) {
    JS::Value result = JS::GetPromiseResult(promise);
    resArr[1].set(result);
  }

  args.rval().setObjectOrNull(JS::NewArrayObject(cx, resArr));
  return true;
}

static bool getProxyDetails(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JSObject *proxyObj = JS::ToObject(cx, args.get(0));

  // Return undefined if it's not a proxy.
  if (!js::IsScriptedProxy(proxyObj)) { // must be a scripted (JS-defined) proxy
    args.rval().setUndefined();
    return true;
  }

  // Return [target, handler]
  JS::RootedValueArray<2> resArr(cx);
  resArr[0].set(JS::ObjectOrNullValue(js::GetProxyTargetObject(proxyObj)));
  // see `ScriptedProxyHandler::handlerObject`
  //    https://hg.mozilla.org/releases/mozilla-esr102/file/a03fde6/js/src/proxy/ScriptedProxyHandler.cpp#l173
  resArr[1].set(js::GetProxyReservedSlot(proxyObj, 0 /*ScriptedProxyHandler::HANDLER_EXTRA*/));

  args.rval().setObjectOrNull(JS::NewArrayObject(cx, resArr));
  return true;
}

JSFunctionSpec InternalBinding::utils[] = {
  JS_FN("defineGlobal", defineGlobal, /* nargs */ 2, 0),
  JS_FN("isAnyArrayBuffer", isAnyArrayBuffer, 1, 0),
  JS_FN("isPromise", isPromise, 1, 0),
  JS_FN("isRegExp", isRegExp, 1, 0),
  JS_FN("isTypedArray", isTypedArray, 1, 0),
  JS_FN("getPromiseDetails", getPromiseDetails, 1, 0),
  JS_FN("getProxyDetails", getProxyDetails, 1, 0),
  JS_FS_END
};
