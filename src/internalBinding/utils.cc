
#include "include/internalBinding.hh"

#include <jsapi.h>

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

JSFunctionSpec InternalBinding::utils[] = {
  JS_FN("defineGlobal", defineGlobal, /* nargs */ 2, 0),
  JS_FS_END
};
