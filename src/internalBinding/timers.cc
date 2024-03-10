/**
 * @file timers.cc
 * @author Tom Tang (xmader@distributive.network)
 * @brief Implement functions in `internalBinding("timers")`
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 */

#include "include/internalBinding.hh"
#include "include/pyTypeFactory.hh"
#include "include/PyEventLoop.hh"

#include <jsapi.h>

/**
 * See function declarations in python/pythonmonkey/builtin_modules/internal-binding.d.ts :
 *    `declare function internalBinding(namespace: "timers")`
 */

static bool enqueueWithDelay(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::HandleValue jobArgVal = args.get(0);
  double delaySeconds = args.get(1).toNumber();

  // Convert to a Python function
  JS::RootedValue jobArg(cx, jobArgVal);
  PyObject *job = pyTypeFactory(cx, jobArg)->getPyObject();
  // Schedule job to the running Python event-loop
  PyEventLoop loop = PyEventLoop::getRunningLoop();
  if (!loop.initialized()) return false;
  PyEventLoop::AsyncHandle handle = loop.enqueueWithDelay(job, delaySeconds);

  // Return the `timeoutID` to use in `clearTimeout`
  args.rval().setNumber(PyEventLoop::AsyncHandle::getUniqueId(std::move(handle)));
  return true;
}

static bool cancelByTimeoutId(JSContext *cx, unsigned argc, JS::Value *vp) {
  using AsyncHandle = PyEventLoop::AsyncHandle;
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  double timeoutID = args.get(0).toNumber();

  args.rval().setUndefined();

  // Retrieve the AsyncHandle by `timeoutID`
  AsyncHandle *handle = AsyncHandle::fromId((uint32_t)timeoutID);
  if (!handle) return true; // does nothing on invalid timeoutID

  // Cancel this job on the Python event-loop
  handle->cancel();

  return true;
}

JSFunctionSpec InternalBinding::timers[] = {
  JS_FN("enqueueWithDelay", enqueueWithDelay, /* nargs */ 2, 0),
  JS_FN("cancelByTimeoutId", cancelByTimeoutId, 1, 0),
  JS_FS_END
};
