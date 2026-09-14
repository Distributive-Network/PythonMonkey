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

// LOCAL PATCH (SpiderMonkey 157a1 API change): getHostDefinedData gained a
// second out-param, incumbentGlobal (previously that concept was only
// supplied as an *input* to enqueuePromiseJob below, which this class
// already ignores -- it doesn't track incumbent globals at all). Mechanical
// fix, not a judgment call: set the new param to nullptr too, matching the
// exact same "we don't need this" stance already taken for the original
// `data` param immediately below.
bool JobQueue::getHostDefinedData(JSContext *cx, JS::MutableHandle<JSObject *> incumbentGlobal, JS::MutableHandle<JSObject *> data) const {
  incumbentGlobal.set(nullptr); // We don't need the incumbent global
  data.set(nullptr); // We don't need the host defined data
  return true; // `true` indicates no error
}

// LOCAL PATCH (SpiderMonkey 157a1 API change): see the long comment on
// getHostDefinedGlobal() in JobQueue.hh -- this is a strictly "we don't
// track this" stance, matching InternalJobQueue::getHostDefinedGlobal in
// SpiderMonkey's own reference embedding (js/src/vm/JSContext.cpp).
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

  // LOCAL PATCH (SpiderMonkey 157a1 API change): running this microtask may
  // itself have enqueued further jobs into cx->microTaskQueues (the classic
  // case: the next `await` continuation inside an async function body).
  // Nothing else will pull those out and forward them to Python unless we
  // explicitly re-checkpoint here -- discovered via a real hang (a promise
  // chain with two `await`s stalled after the first hop) when this call was
  // initially missing. See also the two analogous calls in PromiseType.cc,
  // for the other two places new jobs get enqueued outside of a top-level
  // JS_ExecuteScript() call.
  js::RunJobs(cx);

  if (!ok) {
    setSpiderMonkeyException(cx);
    return NULL; // propagates as a Python exception; PyEventLoop's own
                 // eventLoopJobWrapper surfaces it to the loop's exception handler
  }
  Py_RETURN_NONE;
}

static PyMethodDef runMicroTaskCallbackDef = {"JsMicroTaskCallable", runMicroTaskCallback, METH_NOARGS, NULL};

// LOCAL PATCH (SpiderMonkey 157a1 API change): see the long comment on
// runJobs() in JobQueue.hh for why this is no longer a no-op. In short:
// SpiderMonkey now owns the actual job queue (cx->microTaskQueues) and
// expects the embedding to pull jobs from it here, rather than pushing
// each job to the embedding as it's created (the old enqueuePromiseJob
// design). This drains whatever is currently queued and forwards each job
// to the Python event-loop exactly as enqueuePromiseJob used to.
//
// NEEDS REVIEW: this is an architecture change, not a mechanical signature
// fix. Two things in particular haven't been independently verified against
// SpiderMonkey's actual internals: (1) that draining once per top-level
// JS_ExecuteScript() call (see pythonmonkey.cc) is the correct/only place
// a "microtask checkpoint" needs to happen for this embedding's use cases;
// (2) GC-safety of rooting a JSMicroTask* (a plain JSObject*) across the
// gap between dequeuing it here and Python's event-loop actually calling
// runMicroTaskCallback -- modelled on the existing, working
// finalizationRegistryCallbacks/PersistentRooted pattern in this same file,
// but not traced through SpiderMonkey's GC to confirm a JSMicroTask has no
// unusual rooting requirements beyond a normal JSObject*.
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
  // LOCAL PATCH (SpiderMonkey 157a1 API change): see the long comment on
  // dispatchToEventLoop()/delayedDispatchToEventLoop() in JobQueue.hh.
  // JS::InitDispatchToEventLoop was replaced by JS::InitAsyncTaskCallbacks,
  // which additionally requires a delayed-dispatch callback; the last two
  // (asyncTaskStarted/FinishedCallback) are optional and left null, as this
  // embedding has no need to track background-task liveness itself.
  JS::InitAsyncTaskCallbacks(cx, dispatchToEventLoop, delayedDispatchToEventLoop, nullptr, nullptr, cx);
  JS::SetPromiseRejectionTrackerCallback(cx, promiseRejectionTracker);
  return true;
}

static PyObject *callDispatchFunc(PyObject *dispatchFuncTuple, PyObject *Py_UNUSED(unused)) {
  JSContext *cx = (JSContext *)PyLong_AsVoidPtr(PyTuple_GetItem(dispatchFuncTuple, 0));
  JS::Dispatchable *dispatchable = (JS::Dispatchable *)PyLong_AsVoidPtr(PyTuple_GetItem(dispatchFuncTuple, 1));
  // LOCAL PATCH (SpiderMonkey 157a1 API change): Dispatchable::run() is now
  // protected; the new public entry point is the static Dispatchable::Run,
  // which also takes (and is responsible for releasing) ownership -- hence
  // reconstructing a UniquePtr from the raw pointer smuggled through the
  // Python closure (see dispatchToEventLoop(), which released it into this
  // same raw form).
  JS::Dispatchable::Run(cx, js::UniquePtr<JS::Dispatchable>(dispatchable), JS::Dispatchable::NotShuttingDown);
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
  // See the long comment on this method's declaration in JobQueue.hh:
  // this embedding has no cross-thread-safe delayed-dispatch mechanism, and
  // js/public/Promise.h explicitly sanctions returning false in that case.
  //
  // When declining a dispatch after taking ownership, the correct call is
  // the public static JS::Dispatchable::ReleaseFailedTask -- NOT
  // transferToRuntime() (a first attempt at this used that instead, going
  // off Dispatchable's doc comment showing its usage pattern, but that
  // comment describes SpiderMonkey's OWN internal usage: transferToRuntime()
  // is `protected`, confirmed by a real build error, so an embedder
  // callback like this one cannot call it directly). Found the actually
  // correct, embedder-facing pattern by reading real production usage in
  // Gecko: dom/workers/RuntimeService.cpp's JSDispatchableRunnable::
  // PostDispatch calls exactly this, in exactly this "we took ownership but
  // failed/declined to dispatch" situation:
  //   JS::Dispatchable::ReleaseFailedTask(std::move(mDispatchable));
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
  // LOCAL PATCH (SpiderMonkey 157a1 removed mfbt's mozilla::Unused/Unused.h
  // entirely -- confirmed absent anywhere in the current mozilla-central
  // tree, not just renamed. mozilla::Unused<<expr was only ever a
  // discard-nodiscard-return-value helper (see its old definition, backed
  // up at _spidermonkey_install.orig-136a1-backup/.../mozilla/Unused.h) --
  // functionally identical to a plain (void) cast.
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