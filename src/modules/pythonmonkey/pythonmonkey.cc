#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include "include/PyType.hh"
#include "include/StrType.hh"
#include "include/FloatType.hh"

#include <jsapi.h>
#include <js/CompilationAndEvaluation.h>
#include <js/Initialization.h>
#include <js/SourceText.h>

#include <Python.h>

static JSContext *cx;             /**< pointer to PythonMonkey's JSContext */
static JS::RootedObject *global;  /**< pointer to the global object of PythonMonkey's JSContext */

typedef std::unordered_map<PyType *, std::vector<JS::PersistentRootedValue *>>::iterator PyToGCIterator;
std::unordered_map<PyType *, std::vector<JS::PersistentRootedValue *>> PyTypeToGCThing; /**< data structure to hold memoized PyObject & GCThing data for handling GC*/

void handleSharedPythonMonkeyMemory(JSContext *cx, JSGCStatus status, JS::GCReason reason, void *data) {
  if (status == JSGCStatus::JSGC_BEGIN) {
    PyToGCIterator pyIt = PyTypeToGCThing.begin();
    while (pyIt != PyTypeToGCThing.end()) {
      // If the PyObject reference count is exactly 1, then the only reference to the object is the one
      // we are holding, which means the object is ready to be freed.
      if (PyObject_GC_IsFinalized(pyIt->first->getPyObject()) || pyIt->first->getPyObject()->ob_refcnt == 1) {
        for (JS::PersistentRootedValue *rval: pyIt->second) { // for each related GCThing
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

static void cleanup() {
  JS_DestroyContext(cx);
  JS_ShutDown();
  delete global;
}

static void memoizePyTypeAndGCThing(PyType *pyType, JS::PersistentRootedValue *GCThing) {
  PyToGCIterator pyIt = PyTypeToGCThing.find(pyType);

  if (pyIt == PyTypeToGCThing.end()) { // if the PythonObject is not memoized
    std::vector<JS::PersistentRootedValue *> gcVector(
      {{GCThing}});
    PyTypeToGCThing.insert({{pyType, gcVector}});
  }
  else {
    pyIt->second.push_back(GCThing);
  }
}

static PyObject *collect(PyObject *self, PyObject *args) {
  JS_GC(cx);
  Py_RETURN_NONE;
}

static PyObject *asUCS4(PyObject *self, PyObject *args) {
  StrType *str = new StrType(PyTuple_GetItem(args, 0));
  if (!PyUnicode_Check(str->getPyObject())) {
    PyErr_SetString(PyExc_TypeError, "pythonmonkey.asUCS4 expects a string as its first argument");
    return NULL;
  }

  return str->asUCS4();
}

static PyObject *eval(PyObject *self, PyObject *args) {

  StrType *code = new StrType(PyTuple_GetItem(args, 0));
  if (!PyUnicode_Check(code->getPyObject())) {
    PyErr_SetString(PyExc_TypeError, "pythonmonkey.eval expects a string as its first argument");
    return NULL;
  }

  JSAutoRealm ar(cx, *global);
  JS::CompileOptions options (cx);
  options.setFileAndLine("noname", 1);

  // initialize JS context
  JS::SourceText<mozilla::Utf8Unit> source;
  if (!source.init(cx, code->getValue(), strlen(code->getValue()), JS::SourceOwnership::Borrowed)) {
    PyErr_SetString(PyExc_RuntimeError, "Spidermonkey could not initialize with given JS code.");
    return NULL;
  }

  // evaluate source code
  JS::PersistentRootedValue *rval = new JS::PersistentRootedValue(cx);
  if (!JS::Evaluate(cx, options, source, rval)) {
    PyErr_SetString(PyExc_RuntimeError, "Spidermonkey could not evaluate the given JS code.");
    return NULL;
  }

  // translate to the proper python type
  PyType *returnValue = NULL;
  if (rval->isString()) {
    returnValue = new StrType(cx, rval->toString());
    memoizePyTypeAndGCThing(returnValue, rval);
  }
  if (rval->isNumber()) {
    returnValue = new FloatType(rval->toNumber());
  }

  if (returnValue) {
    return returnValue->getPyObject();
  }
  else {
    Py_RETURN_NONE;
  }
}

static PyMethodDef PythonMonkeyMethods[] = {
  {"eval", eval, METH_VARARGS, "Javascript evaluator in Python"},
  {"collect", collect, METH_VARARGS, "Calls the spidermonkey garbage collector"},
  {"asUCS4", asUCS4, METH_VARARGS, "Expects a python string in UTF16 encoding, and returns a new equivalent string in UCS4. Undefined behaviour if the string is not in UTF16."},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef pythonmonkey =
{
  PyModuleDef_HEAD_INIT,
  "pythonmonkey",                                   /* name of module */
  "A module for python to JS interoperability", /* module documentation, may be NULL */
  -1,                                           /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
  PythonMonkeyMethods
};

PyMODINIT_FUNC PyInit_pythonmonkey(void)
{
  if (!JS_Init())
    return NULL;

  cx = JS_NewContext(JS::DefaultHeapMaxBytes);
  if (!cx)
    return NULL;

  if (!JS::InitSelfHostedCode(cx))
    return NULL;

  JS::RealmOptions options;
  static JSClass globalClass = {"global", JSCLASS_GLOBAL_FLAGS, &JS::DefaultGlobalClassOps};
  global = new JS::RootedObject(cx, JS_NewGlobalObject(cx, &globalClass, nullptr, JS::FireOnNewGlobalHook, options));
  if (!global)
    return NULL;

  Py_AtExit(cleanup);
  JS_SetGCCallback(cx, handleSharedPythonMonkeyMemory, NULL);
  return PyModule_Create(&pythonmonkey);
}