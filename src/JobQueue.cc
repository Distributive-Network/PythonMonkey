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

#include "include/PyEventLoop.hh"
#include "include/pyTypeFactory.hh"

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
  throw std::logic_error("JobQueue::empty is not implemented\n");
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