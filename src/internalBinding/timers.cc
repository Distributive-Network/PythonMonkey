/**
 * @file timers.cc
 * @author Tom Tang (xmader@distributive.network)
 * @brief Implement functions in `internalBinding("timers")`
 */

#include "include/internalBinding.hh"
#include "include/pyTypeFactory.hh"
#include "include/PyEventLoop.hh"

#include <jsapi.h>

using AsyncHandle = PyEventLoop::AsyncHandle;

/**
 * See function declarations in python/pythonmonkey/builtin_modules/internal-binding.d.ts :
 *    `declare function internalBinding(namespace: "timers")`
 */

static bool enqueueWithDelay(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::HandleValue jobArgVal = args.get(0);
  double delaySeconds = args.get(1).toNumber();

  // Convert to a Python function
  // FIXME (Tom Tang): memory leak, not free-ed
  JS::RootedObject *thisv = new JS::RootedObject(cx, nullptr);
  JS::RootedValue *jobArg = new JS::RootedValue(cx, jobArgVal);
  PyObject *job = pyTypeFactory(cx, thisv, jobArg)->getPyObject();

  // Schedule job to the running Python event-loop
  PyEventLoop loop = PyEventLoop::getRunningLoop();
  if (!loop.initialized()) return false;
  PyEventLoop::AsyncHandle handle = loop.enqueueWithDelay(job, delaySeconds);

  // Return the `timeoutID` to use in `clearTimeout`
  args.rval().setNumber(PyEventLoop::AsyncHandle::getUniqueId(std::move(handle)));
  return true;
}

static bool cancelByTimeoutId(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  double timeoutID = args.get(0).toNumber();

  args.rval().setUndefined();

  // Retrieve the AsyncHandle by `timeoutID`
  AsyncHandle *handle = AsyncHandle::fromId((uint32_t)timeoutID);
  if (!handle) return true; // does nothing on invalid timeoutID

  // Cancel this job on Python event-loop
  handle->cancel();

  return true;
}

static bool timerHasRef(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  double timeoutID = args.get(0).toNumber();

  // Retrieve the AsyncHandle by `timeoutID`
  AsyncHandle *handle = AsyncHandle::fromId((uint32_t)timeoutID);
  if (!handle) return false; // error no such timeoutID

  args.rval().setBoolean(handle->hasRef());
  return true;
}

static bool timerAddRef(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  double timeoutID = args.get(0).toNumber();

  // Retrieve the AsyncHandle by `timeoutID`
  AsyncHandle *handle = AsyncHandle::fromId((uint32_t)timeoutID);
  if (!handle) return false; // error no such timeoutID

  handle->addRef();

  args.rval().setUndefined();
  return true;
}

static bool timerRemoveRef(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  double timeoutID = args.get(0).toNumber();

  // Retrieve the AsyncHandle by `timeoutID`
  AsyncHandle *handle = AsyncHandle::fromId((uint32_t)timeoutID);
  if (!handle) return false; // error no such timeoutID

  handle->removeRef();

  args.rval().setUndefined();
  return true;
}

JSFunctionSpec InternalBinding::timers[] = {
  JS_FN("enqueueWithDelay", enqueueWithDelay, /* nargs */ 2, 0),
  JS_FN("cancelByTimeoutId", cancelByTimeoutId, 1, 0),
  JS_FN("timerHasRef", timerHasRef, 1, 0),
  JS_FN("timerAddRef", timerAddRef, 1, 0),
  JS_FN("timerRemoveRef", timerRemoveRef, 1, 0),
  JS_FS_END
};
