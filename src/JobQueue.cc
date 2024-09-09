/**
 * @file JobQueue.cc
 * @author Tom Tang (xmader@distributive.network)
 * @brief Implements the ECMAScript Job Queue
 * @date 2023-04-03
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/JobQueue.hh"
#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include "include/PyEventLoop.hh"
#include "include/pyTypeFactory.hh"
#include "include/PromiseType.hh"

#include <Python.h>

#include <jsfriendapi.h>
#include <mozilla/Unused.h>

#include <stdexcept>

JobQueue::JobQueue(JSContext *cx) {
  finalizationRegistryCallbacks = new JS::PersistentRooted<FunctionVector>(cx);   // Leaks but it's OK since freed at process exit
}

JSObject *JobQueue::getIncumbentGlobal(JSContext *cx) {
  return JS::CurrentGlobalOrNull(cx);
}

bool JobQueue::enqueuePromiseJob(JSContext *cx,
  [[maybe_unused]] JS::HandleObject promise,
  JS::HandleObject job,
  [[maybe_unused]] JS::HandleObject allocationSite,
  JS::HandleObject incumbentGlobal) {

  // Convert the `job` JS function to a Python function for event-loop callback
  JS::RootedValue jobv(cx, JS::ObjectValue(*job));
  PyObject *callback = pyTypeFactory(cx, jobv);

  // Send job to the running Python event-loop
  PyEventLoop loop = PyEventLoop::getRunningLoop();
  if (!loop.initialized()) return false;

  // Inform the JS runtime that the job queue is no longer empty
  JS::JobQueueMayNotBeEmpty(cx);

  loop.enqueue(callback);

  Py_DECREF(callback);
  return true;
}

void JobQueue::runJobs(JSContext *cx) {
  // Do nothing
}

bool JobQueue::empty() const {
  // TODO (Tom Tang): implement using `get_running_loop` and getting job count on loop???
  return true; // see https://hg.mozilla.org/releases/mozilla-esr128/file/tip/js/src/builtin/Promise.cpp#l6946
}

bool JobQueue::isDrainingStopped() const {
  // TODO (Tom Tang): implement this by detecting if the Python event-loop is still running
  return false;
}

js::UniquePtr<JS::JobQueue::SavedJobQueue> JobQueue::saveJobQueue(JSContext *cx) {
  auto saved = js::MakeUnique<JS::JobQueue::SavedJobQueue>();
  if (!saved) {
    JS_ReportOutOfMemory(cx);
    return NULL;
  }
  return saved;
}

bool JobQueue::init(JSContext *cx) {
  JS::SetJobQueue(cx, this);
  JS::InitDispatchToEventLoop(cx, dispatchToEventLoop, cx);
  JS::SetPromiseRejectionTrackerCallback(cx, promiseRejectionTracker);
  return true;
}

static PyObject *callDispatchFunc(PyObject *dispatchFuncTuple, PyObject *Py_UNUSED(unused)) {
  JSContext *cx = (JSContext *)PyLong_AsVoidPtr(PyTuple_GetItem(dispatchFuncTuple, 0));
  JS::Dispatchable *dispatchable = (JS::Dispatchable *)PyLong_AsVoidPtr(PyTuple_GetItem(dispatchFuncTuple, 1));
  dispatchable->run(cx, JS::Dispatchable::NotShuttingDown);
  Py_RETURN_NONE;
}

static PyMethodDef callDispatchFuncDef = {"JsDispatchCallable", callDispatchFunc, METH_NOARGS, NULL};

bool JobQueue::dispatchToEventLoop(void *closure, JS::Dispatchable *dispatchable) {
  JSContext *cx = (JSContext *)closure;

  // The `dispatchToEventLoop` function is running in a helper thread, so
  // we must acquire the Python GIL (global interpreter lock)
  //    see https://docs.python.org/3/c-api/init.html#non-python-created-threads
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject *dispatchFuncTuple = PyTuple_Pack(2, PyLong_FromVoidPtr(cx), PyLong_FromVoidPtr(dispatchable));
  PyObject *pyFunc = PyCFunction_New(&callDispatchFuncDef, dispatchFuncTuple);

  // Avoid using the current, JS helper thread to send jobs to event-loop as it may cause deadlock
  PyThread_start_new_thread((void (*)(void *)) &sendJobToMainLoop, pyFunc);

  PyGILState_Release(gstate);
  return true;
}

bool sendJobToMainLoop(PyObject *pyFunc) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  // Send job to the running Python event-loop on cx's thread (the main thread)
  PyEventLoop loop = PyEventLoop::getMainLoop();
  if (!loop.initialized()) {
    PyGILState_Release(gstate);
    return false;
  }
  loop.enqueue(pyFunc);

  PyGILState_Release(gstate);
  return true;
}

void JobQueue::promiseRejectionTracker(JSContext *cx,
  bool mutedErrors,
  JS::HandleObject promise,
  JS::PromiseRejectionHandlingState state,
  [[maybe_unused]] void *privateData) {

  // We only care about unhandled Promises
  if (state != JS::PromiseRejectionHandlingState::Unhandled) {
    return;
  }
  // If the `mutedErrors` option is set to True in `pm.eval`, eval errors or unhandled rejections should be ignored.
  if (mutedErrors) {
    return;
  }

  // Test if there's no user-defined (or pmjs defined) exception handler on the Python event-loop
  PyEventLoop loop = PyEventLoop::getRunningLoop();
  if (!loop.initialized()) return;
  PyObject *customHandler = PyObject_GetAttrString(loop._loop, "_exception_handler"); // see https://github.com/python/cpython/blob/v3.9.16/Lib/asyncio/base_events.py#L1782
  if (customHandler == Py_None) { // we only have the default exception handler
    // Set an exception handler to the event-loop
    PyObject *pmModule = PyImport_ImportModule("pythonmonkey");
    PyObject *exceptionHandler = PyObject_GetAttrString(pmModule, "simpleUncaughtExceptionHandler");
    PyObject_CallMethod(loop._loop, "set_exception_handler", "O", exceptionHandler);
    Py_DECREF(pmModule);
    Py_DECREF(exceptionHandler);
  }
  Py_DECREF(customHandler);

  // Go ahead and send this unhandled Promise rejection to the exception handler on the Python event-loop
  PyObject *pyFuture = PromiseType::getPyObject(cx, promise); // ref count == 2
  // Unhandled Future object calls the event-loop exception handler in its destructor (the `__del__` magic method)
  // See https://github.com/python/cpython/blob/v3.9.16/Lib/asyncio/futures.py#L108
  //  or https://github.com/python/cpython/blob/v3.9.16/Modules/_asynciomodule.c#L1457-L1467 (It will actually use the C module by default, see futures.py#L417-L423)
  Py_DECREF(pyFuture); // decreasing the reference count from 2 to 1, leaving one for the `onResolved` callback in `PromiseType::getPyObject`, which will be called very soon and clean up the reference
}

void JobQueue::queueFinalizationRegistryCallback(JSFunction *callback) {
  mozilla::Unused << finalizationRegistryCallbacks->append(callback);
}

bool JobQueue::runFinalizationRegistryCallbacks(JSContext *cx) {
  bool ranCallbacks = false;
  JS::Rooted<FunctionVector> callbacks(cx);
  std::swap(callbacks.get(), finalizationRegistryCallbacks->get());
  for (JSFunction *f: callbacks) {
    JS::ExposeObjectToActiveJS(JS_GetFunctionObject(f));

    JSAutoRealm ar(cx, JS_GetFunctionObject(f));
    JS::RootedFunction func(cx, f);
    JS::RootedValue unused_rval(cx);
    // we don't raise an exception here because there is nowhere to catch it
    mozilla::Unused << JS_CallFunction(cx, NULL, func, JS::HandleValueArray::empty(), &unused_rval);
    ranCallbacks = true;
  }

  return ranCallbacks;
}