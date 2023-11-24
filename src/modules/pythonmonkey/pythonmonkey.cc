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
#include "include/JSObjectProxy.hh"
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

#include <unordered_map>
#include <vector>

typedef std::unordered_map<PyType *, std::vector<JS::PersistentRooted<JS::Value> *>>::iterator PyToGCIterator;
typedef struct {
  PyObject_HEAD
} NullObject;

std::unordered_map<PyType *, std::vector<JS::PersistentRooted<JS::Value> *>> PyTypeToGCThing; /**< data structure to hold memoized PyObject & GCThing data for handling GC*/

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

static void cleanup() {
  delete autoRealm;
  delete global;
  delete JOB_QUEUE;
  if (GLOBAL_CX) JS_DestroyContext(GLOBAL_CX);
  JS_ShutDown();
}

void memoizePyTypeAndGCThing(PyType *pyType, JS::Handle<JS::Value> GCThing) {
  JS::PersistentRooted<JS::Value> *RootedGCThing = new JS::PersistentRooted<JS::Value>(GLOBAL_CX, GCThing);
  PyToGCIterator pyIt = PyTypeToGCThing.find(pyType);

  if (pyIt == PyTypeToGCThing.end()) { // if the PythonObject is not memoized
    std::vector<JS::PersistentRooted<JS::Value> *> gcVector(
      {{RootedGCThing}});
    PyTypeToGCThing.insert({{pyType, gcVector}});
  }
  else {
    pyIt->second.push_back(RootedGCThing);
  }
}

void handleSharedPythonMonkeyMemory(JSContext *cx, JSGCStatus status, JS::GCReason reason, void *data) {
  if (status == JSGCStatus::JSGC_BEGIN) {
    PyToGCIterator pyIt = PyTypeToGCThing.begin();
    while (pyIt != PyTypeToGCThing.end()) {
      PyObject *pyObj = pyIt->first->getPyObject();
      // If the PyObject reference count is exactly 1, then the only reference to the object is the one
      // we are holding, which means the object is ready to be free'd.
      if (_PyGC_FINALIZED(pyObj) || pyObj->ob_refcnt == 1) { // PyObject_GC_IsFinalized is only available in Python 3.9+
        for (JS::PersistentRooted<JS::Value> *rval: pyIt->second) { // for each related GCThing
          bool found = false;
          for (PyToGCIterator innerPyIt = PyTypeToGCThing.begin(); innerPyIt != PyTypeToGCThing.end(); innerPyIt++) { // for each other PyType pointer
            if (innerPyIt != pyIt && std::find(innerPyIt->second.begin(), innerPyIt->second.end(), rval) != innerPyIt->second.end()) { // if the PyType is also related to the GCThing
              found = true;
              break;
            }
          }
          // if this PyObject is the last PyObject that references this GCThing, then the GCThing can also be free'd
          if (!found) {
            delete rval;
          }
        }
        pyIt = PyTypeToGCThing.erase(pyIt);
      }
      else {
        pyIt++;
      }
    }
  }
};

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

  JS::RealmOptions options;
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

  JS_SetGCCallback(GLOBAL_CX, handleSharedPythonMonkeyMemory, NULL);
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

  if (PyModule_AddObject(pyModule, "SpiderMonkeyError", SpiderMonkeyError)) {
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

  return pyModule;
}
