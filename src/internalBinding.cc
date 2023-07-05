
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
  } else { // not found
    return nullptr;
  }
}

/**
 * @brief Implement the `internalBinding(namespace)` function
 */
static PyObject *internalBindingFn(PyObject *cxPointer, PyObject *args) {
  JSContext *cx = (JSContext *)PyLong_AsVoidPtr(cxPointer);

  // Get the `namespace` argument as string
  const char *namespaceChars = PyUnicode_AsUTF8(PyTuple_GetItem(args, 0));
  JS::ConstUTF8CharsZ utf8Chars(namespaceChars, strlen(namespaceChars));
  JSLinearString *namespaceStr = JS_EnsureLinearString(cx, JS_NewStringCopyUTF8Z(cx, utf8Chars));

  // Get the internal bindings by namespace
  JS::RootedObject bindings(cx, getInternalBindingsByNamespace(cx, namespaceStr));

  // Convert to a Python dict
  JS::RootedValue jsVal = JS::RootedValue(cx, JS::ObjectValue(*bindings));
  return pyTypeFactory(cx, nullptr, &jsVal)->getPyObject();
}

static PyMethodDef internalBindingFuncDef = {"internalBinding", internalBindingFn, METH_VARARGS, NULL};

/**
 * @brief Get a usable `internalBinding(namespace)` function in Python
 */
PyObject *getInternalBindingPyFn(JSContext *cx) {
  return PyCFunction_New(&internalBindingFuncDef, PyLong_FromVoidPtr(cx));
}
