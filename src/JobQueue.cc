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
#include "include/setSpiderMonkeyException.hh"

#include <Python.h>

#include <jsfriendapi.h>
#include <js/friend/MicroTask.h>

#include <stdexcept>

JobQueue::JobQueue(JSContext *cx) {
  finalizationRegistryCallbacks = new JS::PersistentRooted<FunctionVector>(cx);   // Leaks but it's OK since freed at process exit
}

bool JobQueue::getHostDefinedData(JSContext *cx, JS::MutableHandle<JSObject *> incumbentGlobal, JS::MutableHandle<JSObject *> data) const {
  incumbentGlobal.set(nullptr); // We don't need the incumbent global
  data.set(nullptr); // We don't need the host defined data
  return true; // `true` indicates no error
}

bool JobQueue::getHostDefinedGlobal(JSContext *cx, JS::MutableHandle<JSObject *> out) const {
  out.set(nullptr);
  return true;
}

// The PyCFunction invoked by the Python event-loop once it's ready to run a
// single deferred JS microtask. `closure` is a 2-tuple of (JSContext*,
// JS::PersistentRooted<JSObject*>* job), both smuggled through as PyLong
// pointers the same way JobQueue::dispatchToEventLoop's callDispatchFunc
// does below for JS::Dispatchable.
static PyObject *runMicroTaskCallback(PyObject *closure, PyObject *Py_UNUSED(unused)) {
  JSContext *cx = (JSContext *)PyLong_AsVoidPtr(PyTuple_GetItem(closure, 0));
  auto *rootedJob = (JS::PersistentRooted<JSObject *> *)PyLong_AsVoidPtr(PyTuple_GetItem(closure, 1));

  JS::Rooted<JS::JSMicroTask *> job(cx, rootedJob->get());
  delete rootedJob; // the PersistentRooted was only needed to keep `job` alive until now

  bool ok = true;
  JSObject *global = JS::GetExecutionGlobalFromJSMicroTask(job);
  if (global) {
    JSAutoRealm ar(cx, global);
    ok = JS::RunJSMicroTask(cx, job);
  }

  // Running this microtask may enqueue the next one in an await chain --
  // re-checkpoint so it doesn't just sit there. See also PromiseType.cc.
  js::RunJobs(cx);

  if (!ok) {
    setSpiderMonkeyException(cx);
    return NULL; // propagates as a Python exception; PyEventLoop's own
                 // eventLoopJobWrapper surfaces it to the loop's exception handler
  }
  Py_RETURN_NONE;
}

static PyMethodDef runMicroTaskCallbackDef = {"JsMicroTaskCallable", runMicroTaskCallback, METH_NOARGS, NULL};

// NEEDS REVIEW: GC-safety of rooting a JSMicroTask* across the gap between
// dequeuing it here and the Python event-loop calling runMicroTaskCallback
// is modelled on the finalizationRegistryCallbacks pattern below, but not
// independently verified for JSMicroTask specifically. Also unverified:
// that draining once per top-level JS_ExecuteScript() (pythonmonkey.cc) is
// the only place a checkpoint is needed for this embedding.
void JobQueue::runJobs(JSContext *cx) {
  while (JS::HasAnyMicroTasks(cx)) {
    JS::RootedValue entry(cx, JS::DequeueNextMicroTask(cx));
    if (entry.isNull()) {
      break;
    }

    JS::Rooted<JS::JSMicroTask *> job(cx, JS::ToMaybeWrappedJSMicroTask(entry));
    if (!job) {
      continue; // not a JS microtask; nothing we support runs these
    }

    // Root the job on the heap so it survives until the Python event-loop
    // calls back into us, which may be well after this function returns.
    auto *rootedJob = new JS::PersistentRooted<JSObject *>(cx, job);

    PyObject *cxArg = PyLong_FromVoidPtr(cx);
    PyObject *jobArg = PyLong_FromVoidPtr(rootedJob);
    PyObject *closure = PyTuple_Pack(2, cxArg, jobArg);
    Py_DECREF(cxArg);
    Py_DECREF(jobArg);
    PyObject *callback = PyCFunction_New(&runMicroTaskCallbackDef, closure);
    Py_DECREF(closure);

    PyEventLoop loop = PyEventLoop::getRunningLoop();
    if (!loop.initialized()) {
      delete rootedJob;
      Py_DECREF(callback);
      return;
    }

    // Inform the JS runtime that the job queue is no longer empty
    JS::JobQueueMayNotBeEmpty(cx);

    loop.enqueue(callback);
    Py_DECREF(callback);
  }
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
  // Last two args (asyncTaskStarted/FinishedCallback) are optional; this
  // embedding doesn't need to track background-task liveness.
  JS::InitAsyncTaskCallbacks(cx, dispatchToEventLoop, delayedDispatchToEventLoop, nullptr, nullptr, cx);
  JS::SetPromiseRejectionTrackerCallback(cx, promiseRejectionTracker);
  return true;
}

static PyObject *callDispatchFunc(PyObject *dispatchFuncTuple, PyObject *Py_UNUSED(unused)) {
  JSContext *cx = (JSContext *)PyLong_AsVoidPtr(PyTuple_GetItem(dispatchFuncTuple, 0));
  JS::Dispatchable *dispatchable = (JS::Dispatchable *)PyLong_AsVoidPtr(PyTuple_GetItem(dispatchFuncTuple, 1));
  // Dispatchable::run() is protected; reconstruct the UniquePtr released
  // into raw form by dispatchToEventLoop() below and run it via Run().
  JS::Dispatchable::Run(cx, js::UniquePtr<JS::Dispatchable>(dispatchable), JS::Dispatchable::NotShuttingDown);

  // This resumes JS execution (e.g. finishing an off-thread WebAssembly
  // compile/instantiate), which can settle promises and enqueue reaction
  // jobs -- same as the other checkpoints in this file, nothing else drains
  // this one. Without it, `await WebAssembly.instantiate(...)` hangs forever
  // even though the dispatchable itself ran successfully.
  js::RunJobs(cx);

  Py_RETURN_NONE;
}

static PyMethodDef callDispatchFuncDef = {"JsDispatchCallable", callDispatchFunc, METH_NOARGS, NULL};

bool JobQueue::dispatchToEventLoop(void *closure, js::UniquePtr<JS::Dispatchable> &&dispatchable) {
  JSContext *cx = (JSContext *)closure;

  // The `dispatchToEventLoop` function is running in a helper thread, so
  // we must acquire the Python GIL (global interpreter lock)
  //    see https://docs.python.org/3/c-api/init.html#non-python-created-threads
  PyGILState_STATE gstate = PyGILState_Ensure();

  // Release ownership into a raw pointer to smuggle it through the Python
  // closure; reclaimed by callDispatchFunc via Dispatchable::Run above.
  JS::Dispatchable *raw = dispatchable.release();
  PyObject *dispatchFuncTuple = PyTuple_Pack(2, PyLong_FromVoidPtr(cx), PyLong_FromVoidPtr(raw));
  PyObject *pyFunc = PyCFunction_New(&callDispatchFuncDef, dispatchFuncTuple);

  // Avoid using the current, JS helper thread to send jobs to event-loop as it may cause deadlock
  PyThread_start_new_thread((void (*)(void *)) &sendJobToMainLoop, pyFunc);

  PyGILState_Release(gstate);
  return true;
}

bool JobQueue::delayedDispatchToEventLoop(void *closure, js::UniquePtr<JS::Dispatchable> &&dispatchable, uint32_t delay) {
  // No cross-thread-safe delayed-dispatch mechanism here (see JobQueue.hh).
  // ReleaseFailedTask is the embedder-facing way to decline after taking
  // ownership -- transferToRuntime() is SpiderMonkey's own internal use
  // and is protected.
  JS::Dispatchable::ReleaseFailedTask(std::move(dispatchable));
  return false;
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

  loop._loop = nullptr; // the `Py_XDECREF` Python API call in `PyEventLoop`'s destructor will not be accessible once we hand over the GIL by `PyGILState_Release`
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
  // mozilla::Unused (mfbt) was removed upstream; it was just a discard cast.
  (void)finalizationRegistryCallbacks->append(callback);
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
    (void)JS_CallFunction(cx, NULL, func, JS::HandleValueArray::empty(), &unused_rval);
    ranCallbacks = true;
  }

  return ranCallbacks;
}