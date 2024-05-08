/**
 * @file internalBinding.hh
 * @author Tom Tang (xmader@distributive.network)
 * @brief
 * @date 2023-05-16
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include <jsapi.h>
#include <Python.h>

namespace InternalBinding {
  extern JSFunctionSpec utils[];
  extern JSFunctionSpec timers[];
}

JSObject *createInternalBindingsForNamespace(JSContext *cx, JSFunctionSpec *methodSpecs);
JSObject *getInternalBindingsByNamespace(JSContext *cx, JSLinearString *namespaceStr);

JSFunction *createInternalBinding(JSContext *cx);
PyObject *getInternalBindingPyFn(JSContext *cx);
