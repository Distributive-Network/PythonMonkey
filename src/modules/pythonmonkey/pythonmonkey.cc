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
#include "include/JSArrayIterProxy.hh"
#include "include/JSArrayProxy.hh"
#include "include/JSObjectIterProxy.hh"
#include "include/JSObjectKeysProxy.hh"
#include "include/JSObjectValuesProxy.hh"
#include "include/JSObjectItemsProxy.hh"
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
  .tp_itemsize = 0,
  .tp_dealloc = (destructor)JSObjectProxyMethodDefinitions::JSObjectProxy_dealloc,
  .tp_repr = (reprfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_repr,
  .tp_as_number = &JSObjectProxy_number_methods,
  .tp_as_sequence = &JSObjectProxy_sequence_methods,
  .tp_as_mapping = &JSObjectProxy_mapping_methods,
  .tp_hash = PyObject_HashNotImplemented,
  .tp_getattro = (getattrofunc)JSObjectProxyMethodDefinitions::JSObjectProxy_get,
  .tp_setattro = (setattrofunc)JSObjectProxyMethodDefinitions::JSObjectProxy_assign,
  .tp_flags = Py_TPFLAGS_DEFAULT
  | Py_TPFLAGS_DICT_SUBCLASS,  // https://docs.python.org/3/c-api/typeobj.html#Py_TPFLAGS_DICT_SUBCLASS
  .tp_doc = PyDoc_STR("Javascript Object proxy dict"),
  .tp_richcompare = (richcmpfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_richcompare,
  .tp_iter = (getiterfunc)JSObjectProxyMethodDefinitions::JSObjectProxy_iter,
  .tp_methods = JSObjectProxy_methods,
  .tp_base = &PyDict_Type,
  .tp_init = (initproc)JSObjectProxyMethodDefinitions::JSObjectProxy_init,
  .tp_new = JSObjectProxyMethodDefinitions::JSObjectProxy_new,
};

PyTypeObject JSArrayProxyType = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pythonmonkey.JSArrayProxy",
  .tp_basicsize = sizeof(JSArrayProxy),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor)JSArrayProxyMethodDefinitions::JSArrayProxy_dealloc,
  .tp_repr = (reprfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_repr,
  .tp_as_sequence = &JSArrayProxy_sequence_methods,
  .tp_as_mapping = &JSArrayProxy_mapping_methods,
  .tp_getattro = (getattrofunc)JSArrayProxyMethodDefinitions::JSArrayProxy_get,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DICT_SUBCLASS,
  .tp_doc = PyDoc_STR("Javascript Array proxy list"),
  .tp_traverse = (traverseproc)JSArrayProxyMethodDefinitions::JSArrayProxy_traverse,
  .tp_clear = (inquiry)JSArrayProxyMethodDefinitions::JSArrayProxy_clear_slot,
  .tp_richcompare = (richcmpfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_richcompare,
  .tp_iter = (getiterfunc)JSArrayProxyMethodDefinitions::JSArrayProxy_iter,
  .tp_methods = JSArrayProxy_methods,
  .tp_base = &PyList_Type,
  .tp_init = (initproc)JSArrayProxyMethodDefinitions::JSArrayProxy_init,
  .tp_new = JSArrayProxyMethodDefinitions::JSArrayProxy_new,
};

PyTypeObject JSArrayIterProxyType = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pythonmonkey.JSArrayIterProxy",
  .tp_basicsize = sizeof(JSArrayIterProxy),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor)JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_dealloc,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
  .tp_doc = PyDoc_STR("Javascript Array proxy iterator"),
  .tp_traverse =  (traverseproc)JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_traverse,
  .tp_iter = (getiterfunc)JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_iter,
  .tp_iternext = (iternextfunc)JSArrayIterProxyMethodDefinitions::JSArrayIterProxy_next,
  .tp_methods = JSArrayIterProxy_methods,
  .tp_base = &PyListIter_Type
};

PyTypeObject JSObjectIterProxyType = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pythonmonkey.JSObjectIterProxy",
  .tp_basicsize = sizeof(JSObjectIterProxy),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor)JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_dealloc,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
  .tp_doc = PyDoc_STR("Javascript Object proxy key iterator"),
  .tp_traverse =  (traverseproc)JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_traverse,
  .tp_iter = (getiterfunc)JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_iter,
  .tp_iternext = (iternextfunc)JSObjectIterProxyMethodDefinitions::JSObjectIterProxy_nextkey,
  .tp_methods = JSObjectIterProxy_methods,
  .tp_base = &PyDictIterKey_Type
};

PyTypeObject JSObjectKeysProxyType = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pythonmonkey.JSObjectKeysProxy",
  .tp_basicsize = sizeof(JSObjectKeysProxy),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor)JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_dealloc,
  .tp_repr = (reprfunc)JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_repr,
  .tp_as_number = &JSObjectKeysProxy_number_methods,
  .tp_as_sequence = &JSObjectKeysProxy_sequence_methods,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
  .tp_doc = PyDoc_STR("Javascript Object Keys proxy"),
  .tp_traverse =  (traverseproc)JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_traverse,
  .tp_richcompare = (richcmpfunc)JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_richcompare,
  .tp_iter = (getiterfunc)JSObjectKeysProxyMethodDefinitions::JSObjectKeysProxy_iter,
  .tp_methods = JSObjectKeysProxy_methods,
  .tp_getset = JSObjectKeysProxy_getset,
  .tp_base = &PyDictKeys_Type
};

PyTypeObject JSObjectValuesProxyType = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pythonmonkey.JSObjectValuesProxy",
  .tp_basicsize = sizeof(JSObjectValuesProxy),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor)JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_dealloc,
  .tp_repr = (reprfunc)JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_repr,
  .tp_as_sequence = &JSObjectValuesProxy_sequence_methods,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
  .tp_doc = PyDoc_STR("Javascript Object Values proxy"),
  .tp_traverse =  (traverseproc)JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_traverse,
  .tp_iter = (getiterfunc)JSObjectValuesProxyMethodDefinitions::JSObjectValuesProxy_iter,
  .tp_methods = JSObjectValuesProxy_methods,
  .tp_getset = JSObjectValuesProxy_getset,
  .tp_base = &PyDictValues_Type
};

PyTypeObject JSObjectItemsProxyType = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pythonmonkey.JSObjectItemsProxy",
  .tp_basicsize = sizeof(JSObjectItemsProxy),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor)JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_dealloc,
  .tp_repr = (reprfunc)JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_repr,
  // .tp_as_number = defaults are fine
  .tp_as_sequence = &JSObjectItemsProxy_sequence_methods,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
  .tp_doc = PyDoc_STR("Javascript Object Items proxy"),
  .tp_traverse =  (traverseproc)JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_traverse,
  // .tp_richcompare = TODO tuple support
  .tp_iter = (getiterfunc)JSObjectItemsProxyMethodDefinitions::JSObjectItemsProxy_iter,
  .tp_methods = JSObjectItemsProxy_methods,
  .tp_getset = JSObjectItemsProxy_getset,
  .tp_base = &PyDictKeys_Type
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

  JS::RealmCreationOptions creationOptions = JS::RealmCreationOptions();
  JS::RealmBehaviors behaviours = JS::RealmBehaviors();
  creationOptions.setIteratorHelpersEnabled(true);
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
  if (PyType_Ready(&JSArrayProxyType) < 0)
    return NULL;
  if (PyType_Ready(&JSArrayIterProxyType) < 0)
    return NULL;
  if (PyType_Ready(&JSObjectIterProxyType) < 0)
    return NULL;
  if (PyType_Ready(&JSObjectKeysProxyType) < 0)
    return NULL;
  if (PyType_Ready(&JSObjectValuesProxyType) < 0)
    return NULL;
  if (PyType_Ready(&JSObjectItemsProxyType) < 0)
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

  Py_INCREF(&JSArrayProxyType);
  if (PyModule_AddObject(pyModule, "JSArrayProxy", (PyObject *)&JSArrayProxyType) < 0) {
    Py_DECREF(&JSArrayProxyType);
    Py_DECREF(pyModule);
    return NULL;
  }

  Py_INCREF(&JSArrayIterProxyType);
  if (PyModule_AddObject(pyModule, "JSArrayIterProxy", (PyObject *)&JSArrayIterProxyType) < 0) {
    Py_DECREF(&JSArrayIterProxyType);
    Py_DECREF(pyModule);
    return NULL;
  }

  Py_INCREF(&JSObjectIterProxyType);
  if (PyModule_AddObject(pyModule, "JSObjectIterProxy", (PyObject *)&JSObjectIterProxyType) < 0) {
    Py_DECREF(&JSObjectIterProxyType);
    Py_DECREF(pyModule);
    return NULL;
  }

  Py_INCREF(&JSObjectKeysProxyType);
  if (PyModule_AddObject(pyModule, "JSObjectKeysProxy", (PyObject *)&JSObjectKeysProxyType) < 0) {
    Py_DECREF(&JSObjectKeysProxyType);
    Py_DECREF(pyModule);
    return NULL;
  }

  Py_INCREF(&JSObjectValuesProxyType);
  if (PyModule_AddObject(pyModule, "JSObjectValuesProxy", (PyObject *)&JSObjectValuesProxyType) < 0) {
    Py_DECREF(&JSObjectValuesProxyType);
    Py_DECREF(pyModule);
    return NULL;
  }

  Py_INCREF(&JSObjectItemsProxyType);
  if (PyModule_AddObject(pyModule, "JSObjectItemsProxy", (PyObject *)&JSObjectItemsProxyType) < 0) {
    Py_DECREF(&JSObjectItemsProxyType);
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
