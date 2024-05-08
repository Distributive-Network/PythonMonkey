/**
 * @file internalBinding.cc
 * @author Tom Tang (xmader@distributive.network)
 * @brief Create internal bindings to get C++-implemented functions in JS, (imported from NodeJS internal design decisions)
 *        See function declarations in python/pythonmonkey/builtin_modules/internal-binding.d.ts
 * @copyright Copyright (c) 2023 Distributive Corp.
 */

#include "include/internalBinding.hh"
#include "include/pyTypeFactory.hh"

#include <jsapi.h>
#include <js/String.h>
#include <Python.h>

JSObject *createInternalBindingsForNamespace(JSContext *cx, JSFunctionSpec *methodSpecs) {
  JS::RootedObject namespaceObj(cx, JS_NewObjectWithGivenProto(cx, nullptr, nullptr)); // namespaceObj = Object.create(null)
  if (!JS_DefineFunctions(cx, namespaceObj, methodSpecs)) { return nullptr; }
  return namespaceObj;
}

// TODO (Tom Tang): figure out a better way to register InternalBindings to namespace
JSObject *getInternalBindingsByNamespace(JSContext *cx, JSLinearString *namespaceStr) {
  if (JS_LinearStringEqualsLiteral(namespaceStr, "utils")) {
    return createInternalBindingsForNamespace(cx, InternalBinding::utils);
  } else if (JS_LinearStringEqualsLiteral(namespaceStr, "timers")) {
    return createInternalBindingsForNamespace(cx, InternalBinding::timers);
  } else { // not found
    return nullptr;
  }
}

/**
 * @brief Implement the `internalBinding(namespace)` function
 */
static bool internalBindingFn(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Get the `namespace` argument as string
  JS::HandleValue namespaceStrArg = args.get(0);
  JSLinearString *namespaceStr = JS_EnsureLinearString(cx, namespaceStrArg.toString());

  args.rval().setObjectOrNull(getInternalBindingsByNamespace(cx, namespaceStr));
  return true;
}

/**
 * @brief Create the JS `internalBinding` function
 */
JSFunction *createInternalBinding(JSContext *cx) {
  return JS_NewFunction(cx, internalBindingFn, 1, 0, "internalBinding");
}

/**
 * @brief Convert the `internalBinding(namespace)` function to a Python function
 */
PyObject *getInternalBindingPyFn(JSContext *cx) {
  // Create the JS `internalBinding` function
  JSObject *jsFn = (JSObject *)createInternalBinding(cx);

  // Convert to a Python function
  JS::RootedValue jsFnVal(cx, JS::ObjectValue(*jsFn));
  return pyTypeFactory(cx, jsFnVal);
}