/**
 * @file pythonmonkey.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief This file defines the pythonmonkey module, along with its various functions.
 * @version 0.1
 * @date 2023-03-29
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include "include/BoolType.hh"
#include "include/setSpiderMonkeyException.hh"
#include "include/DateType.hh"
#include "include/FloatType.hh"
#include "include/FuncType.hh"
#include "include/JSFunctionProxy.hh"
#include "include/JSMethodProxy.hh"
#include "include/JSObjectProxy.hh"
#include "include/JSStringProxy.hh"
#include "include/PyType.hh"
#include "include/pyTypeFactory.hh"
#include "include/StrType.hh"
#include "include/PyEventLoop.hh"
#include "include/internalBinding.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/friend/ErrorMessages.h>
#include <js/friend/DOMProxy.h>
#include <js/CompilationAndEvaluation.h>
#include <js/ContextOptions.h>
#include <js/Class.h>
#include <js/Date.h>
#include <js/Initialization.h>
#include <js/Object.h>
#include <js/Proxy.h>
#include <js/SourceText.h>
#include <js/Symbol.h>

#include <Python.h>
#include <datetime.h>

JSContext *GLOBAL_CX;

JS::PersistentRootedObject *jsFunctionRegistry;

bool functionRegistryCallback(JSContext *cx, unsigned int argc, JS::Value *vp) {
  JS::CallArgs callargs = JS::CallArgsFromVp(argc, vp);
  Py_DECREF((PyObject *)callargs[0].toPrivate());
  return true;
}

typedef struct {
  PyObject_HEAD
} NullObject;

static PyTypeObject NullType = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pythonmonkey.null",
  .tp_basicsize = sizeof(NullObject),
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_doc = PyDoc_STR("Javascript null object"),
};

static PyTypeObject BigIntType = {
  .tp_name = "pythonmonkey.bigint",
  .tp_flags = Py_TPFLAGS_DEFAULT
  | Py_TPFLAGS_LONG_SUBCLASS // https://docs.python.org/3/c-api/typeobj.html#Py_TPFLAGS_LONG_SUBCLASS
  | Py_TPFLAGS_BASETYPE,     // can be subclassed
  .tp_doc = PyDoc_STR("Javascript BigInt object"),
  .tp_base = &PyLong_Type,   // extending the builtin int type
};

PyTypeObject JSObjectProxyType = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pythonmonkey.JSObjectProxy",
  .tp_basicsize = sizeof(JSObjectProxy),
  .tp_dealloc = (destructor)JSObjectProxyMethodDefinitions::JSObjectProxy_dealloc,
  .tp_repr = (reprfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_repr,
  .tp_as_mapping = &JSObjectProxy_mapping_methods,
  .tp_getattro = (getattrofunc)JSObjectProxyMethodDefinitions::JSObjectProxy_get,
  .tp_setattro = (setattrofunc)JSObjectProxyMethodDefinitions::JSObjectProxy_assign,
  .tp_flags = Py_TPFLAGS_DEFAULT
  | Py_TPFLAGS_DICT_SUBCLASS,  // https://docs.python.org/3/c-api/typeobj.html#Py_TPFLAGS_DICT_SUBCLASS
  .tp_doc = PyDoc_STR("Javascript Object proxy dict"),
  .tp_richcompare = (richcmpfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_richcompare,
  .tp_iter = (getiterfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_iter,
  .tp_base = &PyDict_Type,
  .tp_init = (initproc)JSObjectProxyMethodDefinitions::JSObjectProxy_init,
  .tp_new = JSObjectProxyMethodDefinitions::JSObjectProxy_new,
};

PyTypeObject JSStringProxyType = {
  .tp_name = "pythonmonkey.JSStringProxy",
  .tp_basicsize = sizeof(JSStringProxy),
  .tp_flags = Py_TPFLAGS_DEFAULT
  | Py_TPFLAGS_UNICODE_SUBCLASS // https://docs.python.org/3/c-api/typeobj.html#Py_TPFLAGS_LONG_SUBCLASS
  | Py_TPFLAGS_BASETYPE,     // can be subclassed
  .tp_doc = PyDoc_STR("Javascript String value"),
  .tp_base = &PyUnicode_Type,   // extending the builtin int type
};

PyTypeObject JSFunctionProxyType = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pythonmonkey.JSFunctionProxy",
  .tp_basicsize = sizeof(JSFunctionProxy),
  .tp_dealloc = (destructor)JSFunctionProxyMethodDefinitions::JSFunctionProxy_dealloc,
  .tp_call = JSFunctionProxyMethodDefinitions::JSFunctionProxy_call,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_doc = PyDoc_STR("Javascript Function proxy object"),
  .tp_new = JSFunctionProxyMethodDefinitions::JSFunctionProxy_new,
};

PyTypeObject JSMethodProxyType = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pythonmonkey.JSMethodProxy",
  .tp_basicsize = sizeof(JSMethodProxy),
  .tp_dealloc = (destructor)JSMethodProxyMethodDefinitions::JSMethodProxy_dealloc,
  .tp_call = JSMethodProxyMethodDefinitions::JSMethodProxy_call,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_doc = PyDoc_STR("Javascript Method proxy object"),
  .tp_new = JSMethodProxyMethodDefinitions::JSMethodProxy_new,
};

static void cleanup() {
  delete jsFunctionRegistry;
  delete autoRealm;
  delete global;
  delete JOB_QUEUE;
  if (GLOBAL_CX) JS_DestroyContext(GLOBAL_CX);
  JS_ShutDown();
}

static PyObject *collect(PyObject *self, PyObject *args) {
  JS_GC(GLOBAL_CX);
  Py_RETURN_NONE;
}

static bool getEvalOption(PyObject *evalOptions, const char *optionName, const char **s_p) {
  PyObject *value;

  value = PyDict_GetItemString(evalOptions, optionName);
  if (value)
    *s_p = PyUnicode_AsUTF8(value);
  return value != NULL;
}

static bool getEvalOption(PyObject *evalOptions, const char *optionName, unsigned long *l_p) {
  PyObject *value;

  value = PyDict_GetItemString(evalOptions, optionName);
  if (value)
    *l_p = PyLong_AsUnsignedLong(value);
  return value != NULL;
}

static bool getEvalOption(PyObject *evalOptions, const char *optionName, bool *b_p) {
  PyObject *value;

  value = PyDict_GetItemString(evalOptions, optionName);
  if (value)
    *b_p = PyObject_IsTrue(value) == 1 ? true : false;
  return value != NULL;
}

static PyObject *eval(PyObject *self, PyObject *args) {
  size_t argc = PyTuple_GET_SIZE(args);
  StrType *code = new StrType(PyTuple_GetItem(args, 0));
  PyObject *evalOptions = argc == 2 ? PyTuple_GetItem(args, 1) : NULL;

  if (argc == 0 || !PyUnicode_Check(code->getPyObject())) {
    PyErr_SetString(PyExc_TypeError, "pythonmonkey.eval expects a string as its first argument");
    return NULL;
  }

  if (evalOptions && !PyDict_Check(evalOptions)) {
    PyErr_SetString(PyExc_TypeError, "pythonmonkey.eval expects a dict as its (optional) second argument");
    return NULL;
  }

  JSAutoRealm ar(GLOBAL_CX, *global);
  JS::CompileOptions options (GLOBAL_CX);
  options.setFileAndLine("evaluate", 1)
  .setIsRunOnce(true)
  .setNoScriptRval(false)
  .setIntroductionType("pythonmonkey eval");

  if (evalOptions) {
    const char *s;
    unsigned long l;
    bool b;

    if (getEvalOption(evalOptions, "filename", &s)) options.setFile(s);
    if (getEvalOption(evalOptions, "lineno", &l)) options.setLine(l);
    if (getEvalOption(evalOptions, "column", &l)) options.setColumn(l);
    if (getEvalOption(evalOptions, "mutedErrors", &b)) options.setMutedErrors(b);
    if (getEvalOption(evalOptions, "noScriptRval", &b)) options.setNoScriptRval(b);
    if (getEvalOption(evalOptions, "selfHosting", &b)) options.setSelfHostingMode(b);
    if (getEvalOption(evalOptions, "strict", &b)) if (b) options.setForceStrictMode();
    if (getEvalOption(evalOptions, "module", &b)) if (b) options.setModule();

    if (getEvalOption(evalOptions, "fromPythonFrame", &b) && b) {
#if PY_VERSION_HEX >= 0x03090000
      PyFrameObject *frame = PyEval_GetFrame();
      if (frame && !getEvalOption(evalOptions, "lineno", &l)) {
        options.setLine(PyFrame_GetLineNumber(frame));
      } /* lineno */
#endif
#if 0 && (PY_VERSION_HEX >= 0x030a0000) && (PY_VERSION_HEX < 0x030c0000)
      PyObject *filename = PyDict_GetItemString(frame->f_builtins, "__file__");
#elif (PY_VERSION_HEX >= 0x030c0000)
      PyObject *filename = PyDict_GetItemString(PyFrame_GetGlobals(frame), "__file__");
#else
      PyObject *filename = NULL;
#endif
      if (!getEvalOption(evalOptions, "filename", &s)) {
        if (filename && PyUnicode_Check(filename)) {
          options.setFile(PyUnicode_AsUTF8(filename));
        }
      } /* filename */
    } /* fromPythonFrame */
  } /* eval options */
    // initialize JS context
  JS::SourceText<mozilla::Utf8Unit> source;
  if (!source.init(GLOBAL_CX, code->getValue(), strlen(code->getValue()), JS::SourceOwnership::Borrowed)) {
    setSpiderMonkeyException(GLOBAL_CX);
    return NULL;
  }
  delete code;

  // evaluate source code
  JS::Rooted<JS::Value> *rval = new JS::Rooted<JS::Value>(GLOBAL_CX);
  if (!JS::Evaluate(GLOBAL_CX, options, source, rval)) {
    setSpiderMonkeyException(GLOBAL_CX);
    return NULL;
  }

  // translate to the proper python type
  PyType *returnValue = pyTypeFactory(GLOBAL_CX, global, rval);
  if (PyErr_Occurred()) {
    return NULL;
  }

  // TODO: Find a better way to destroy the root when necessary (when the returned Python object is GCed).
  js::ESClass cls = js::ESClass::Other;   // placeholder if `rval` is not a JSObject
  if (rval->isObject()) {
    JS::GetBuiltinClass(GLOBAL_CX, JS::RootedObject(GLOBAL_CX, &rval->toObject()), &cls);
    if (JS_ObjectIsBoundFunction(&rval->toObject())) {
      cls = js::ESClass::Function; // In SpiderMonkey 115 ESR, bound function is no longer a JSFunction but a js::BoundFunctionObject.
    }
  }
  bool rvalIsFunction = cls == js::ESClass::Function;   // function object
  bool rvalIsString = rval->isString() || cls == js::ESClass::String;   // string primitive or boxed String object
  if (!(rvalIsFunction || rvalIsString)) {   // rval may be a JS function or string which must be kept alive.
    delete rval;
  }

  if (returnValue) {
    return returnValue->getPyObject();
  }
  else {
    Py_RETURN_NONE;
  }
}

static PyObject *waitForEventLoop(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(_)) {
  PyObject *waiter = PyEventLoop::_locker->_queueIsEmpty; // instance of asyncio.Event

  // Making sure it's attached to the current event-loop
  PyEventLoop loop = PyEventLoop::getRunningLoop();
  if (!loop.initialized()) return NULL;
  PyObject_SetAttrString(waiter, "_loop", loop._loop);

  return PyObject_CallMethod(waiter, "wait", NULL);
}

static PyObject *isCompilableUnit(PyObject *self, PyObject *args) {
  StrType *buffer = new StrType(PyTuple_GetItem(args, 0));
  const char *bufferUtf8;
  bool compilable;

  if (!PyUnicode_Check(buffer->getPyObject())) {
    PyErr_SetString(PyExc_TypeError, "pythonmonkey.eval expects a string as its first argument");
    return NULL;
  }

  bufferUtf8 = buffer->getValue();
  compilable = JS_Utf8BufferIsCompilableUnit(GLOBAL_CX, *global, bufferUtf8, strlen(bufferUtf8));

  if (compilable)
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

PyMethodDef PythonMonkeyMethods[] = {
  {"eval", eval, METH_VARARGS, "Javascript evaluator in Python"},
  {"wait", waitForEventLoop, METH_NOARGS, "The event-loop shield. Blocks until all asynchronous jobs finish."},
  {"isCompilableUnit", isCompilableUnit, METH_VARARGS, "Hint if a string might be compilable Javascript"},
  {"collect", collect, METH_VARARGS, "Calls the spidermonkey garbage collector"},
  {NULL, NULL, 0, NULL}
};

struct PyModuleDef pythonmonkey =
{
  PyModuleDef_HEAD_INIT,
  "pythonmonkey",                                   /* name of module */
  "A module for python to JS interoperability",   /* module documentation, may be NULL */
  -1,                                           /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
  PythonMonkeyMethods
};

PyObject *SpiderMonkeyError = NULL;

PyMODINIT_FUNC PyInit_pythonmonkey(void)
{
  if (!PyDateTimeAPI) { PyDateTime_IMPORT; }

  SpiderMonkeyError = PyErr_NewException("pythonmonkey.SpiderMonkeyError", NULL, NULL);
  if (!JS_Init()) {
    PyErr_SetString(SpiderMonkeyError, "Spidermonkey could not be initialized.");
    return NULL;
  }
  Py_AtExit(cleanup);

  GLOBAL_CX = JS_NewContext(JS::DefaultHeapMaxBytes);
  if (!GLOBAL_CX) {
    PyErr_SetString(SpiderMonkeyError, "Spidermonkey could not create a JS context.");
    return NULL;
  }

  JS::ContextOptionsRef(GLOBAL_CX)
  .setWasm(true)
  .setAsmJS(true)
  .setAsyncStack(true)
  .setSourcePragmas(true);

  JOB_QUEUE = new JobQueue();
  if (!JOB_QUEUE->init(GLOBAL_CX)) {
    PyErr_SetString(SpiderMonkeyError, "Spidermonkey could not create the event-loop.");
    return NULL;
  }

  if (!JS::InitSelfHostedCode(GLOBAL_CX)) {
    PyErr_SetString(SpiderMonkeyError, "Spidermonkey could not initialize self-hosted code.");
    return NULL;
  }

  JS::RealmCreationOptions creationOptions = JS::RealmCreationOptions();
  JS::RealmBehaviors behaviours = JS::RealmBehaviors();
  creationOptions.setWeakRefsEnabled(JS::WeakRefSpecifier::EnabledWithCleanupSome); // enable FinalizationRegistry
  JS::RealmOptions options = JS::RealmOptions(creationOptions, behaviours);
  static JSClass globalClass = {"global", JSCLASS_GLOBAL_FLAGS, &JS::DefaultGlobalClassOps};
  global = new JS::RootedObject(GLOBAL_CX, JS_NewGlobalObject(GLOBAL_CX, &globalClass, nullptr, JS::FireOnNewGlobalHook, options));
  if (!global) {
    PyErr_SetString(SpiderMonkeyError, "Spidermonkey could not create a global object.");
    return NULL;
  }

  JS::RootedObject debuggerGlobal(GLOBAL_CX, JS_NewGlobalObject(GLOBAL_CX, &globalClass, nullptr, JS::FireOnNewGlobalHook, options));
  {
    JSAutoRealm r(GLOBAL_CX, debuggerGlobal);
    JS_DefineProperty(GLOBAL_CX, debuggerGlobal, "mainGlobal", *global, JSPROP_READONLY);
    JS_DefineDebuggerObject(GLOBAL_CX, debuggerGlobal);
  }

  autoRealm = new JSAutoRealm(GLOBAL_CX, *global);

  JS_DefineProperty(GLOBAL_CX, *global, "debuggerGlobal", debuggerGlobal, JSPROP_READONLY);

  // XXX: SpiderMonkey bug???
  // In https://hg.mozilla.org/releases/mozilla-esr102/file/3b574e1/js/src/jit/CacheIR.cpp#l317, trying to use the callback returned by `js::GetDOMProxyShadowsCheck()` even it's unset (nullptr)
  // Temporarily solved by explicitly setting the `domProxyShadowsCheck` callback here
  JS::SetDOMProxyInformation(nullptr,
    [](JSContext *, JS::HandleObject, JS::HandleId) {   // domProxyShadowsCheck
      return JS::DOMProxyShadowsResult::ShadowCheckFailed;
    }, nullptr);

  PyObject *pyModule;
  if (PyType_Ready(&NullType) < 0)
    return NULL;
  if (PyType_Ready(&BigIntType) < 0)
    return NULL;
  if (PyType_Ready(&JSObjectProxyType) < 0)
    return NULL;
  if (PyType_Ready(&JSStringProxyType) < 0)
    return NULL;
  if (PyType_Ready(&JSFunctionProxyType) < 0)
    return NULL;
  if (PyType_Ready(&JSMethodProxyType) < 0)
    return NULL;

  pyModule = PyModule_Create(&pythonmonkey);
  if (pyModule == NULL)
    return NULL;

  Py_INCREF(&NullType);
  if (PyModule_AddObject(pyModule, "null", (PyObject *)&NullType) < 0) {
    Py_DECREF(&NullType);
    Py_DECREF(pyModule);
    return NULL;
  }
  Py_INCREF(&BigIntType);
  if (PyModule_AddObject(pyModule, "bigint", (PyObject *)&BigIntType) < 0) {
    Py_DECREF(&BigIntType);
    Py_DECREF(pyModule);
    return NULL;
  }

  Py_INCREF(&JSObjectProxyType);
  if (PyModule_AddObject(pyModule, "JSObjectProxy", (PyObject *)&JSObjectProxyType) < 0) {
    Py_DECREF(&JSObjectProxyType);
    Py_DECREF(pyModule);
    return NULL;
  }

  Py_INCREF(&JSStringProxyType);
  if (PyModule_AddObject(pyModule, "JSStringProxy", (PyObject *)&JSStringProxyType) < 0) {
    Py_DECREF(&JSStringProxyType);
    Py_DECREF(pyModule);
    return NULL;
  }

  Py_INCREF(&JSFunctionProxyType);
  if (PyModule_AddObject(pyModule, "JSFunctionProxy", (PyObject *)&JSFunctionProxyType) < 0) {
    Py_DECREF(&JSFunctionProxyType);
    Py_DECREF(pyModule);
    return NULL;
  }

  Py_INCREF(&JSMethodProxyType);
  if (PyModule_AddObject(pyModule, "JSMethodProxy", (PyObject *)&JSMethodProxyType) < 0) {
    Py_DECREF(&JSMethodProxyType);
    Py_DECREF(pyModule);
    return NULL;
  }

  if (PyModule_AddObject(pyModule, "SpiderMonkeyError", SpiderMonkeyError) < 0) {
    Py_DECREF(pyModule);
    return NULL;
  }

  // Initialize event-loop shield
  PyEventLoop::_locker = new PyEventLoop::Lock();

  PyObject *internalBindingPy = getInternalBindingPyFn(GLOBAL_CX);
  if (PyModule_AddObject(pyModule, "internalBinding", internalBindingPy) < 0) {
    Py_DECREF(internalBindingPy);
    Py_DECREF(pyModule);
    return NULL;
  }

  // initialize FinalizationRegistry of JSFunctions to Python Functions
  JS::RootedValue FinalizationRegistry(GLOBAL_CX);
  JS::RootedObject registryObject(GLOBAL_CX);

  JS_GetProperty(GLOBAL_CX, *global, "FinalizationRegistry", &FinalizationRegistry);
  JS::Rooted<JS::ValueArray<1>> args(GLOBAL_CX);
  JSFunction *registryCallback = JS_NewFunction(GLOBAL_CX, functionRegistryCallback, 1, 0, NULL);
  JS::RootedObject registryCallbackObject(GLOBAL_CX, JS_GetFunctionObject(registryCallback));
  args[0].setObject(*registryCallbackObject);
  if (!JS::Construct(GLOBAL_CX, FinalizationRegistry, args, &registryObject)) {
    setSpiderMonkeyException(GLOBAL_CX);
    return NULL;
  }
  jsFunctionRegistry = new JS::PersistentRootedObject(GLOBAL_CX);
  jsFunctionRegistry->set(registryObject);

  return pyModule;
}
