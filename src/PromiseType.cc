/**
 * @file PromiseType.cc
 * @author Tom Tang (xmader@distributive.network)
 * @brief Struct for representing Promises
 * @version 0.1
 * @date 2023-03-29
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "include/PromiseType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"
#include "include/pyTypeFactory.hh"

#include "include/PyEventLoop.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Promise.h>

#include <Python.h>

#define PY_FUTURE_OBJ_SLOT 20 // (arbitrarily chosen) slot id to access the python object in JS callbacks

static bool onResolvedCb(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Convert the Promise's result to a Python object
  // FIXME (Tom Tang): memory leak, not free-ed
  JS::RootedObject *thisv = new JS::RootedObject(cx, &args.thisv().toObject());
  JS::RootedValue *resultArg = new JS::RootedValue(cx, args[0]);
  PyObject *result = pyTypeFactory(cx, thisv, resultArg)->getPyObject();

  // Get the `asyncio.Future` Python object from function's reserved slot
  JS::Value futureObjVal = js::GetFunctionNativeReserved(&args.callee(), PY_FUTURE_OBJ_SLOT);
  PyObject *futureObj = (PyObject *)(futureObjVal.toPrivate());

  // Settle the Python asyncio.Future by the Promise's result
  PyEventLoop::Future future = PyEventLoop::Future(futureObj);
  future.setResult(result);
}

PromiseType::PromiseType(PyObject *object) : PyType(object) {}

PromiseType::PromiseType(JSContext *cx, JS::HandleObject promise) {
  // Create a python asyncio.Future on the running python event-loop
  PyEventLoop loop = PyEventLoop::getRunningLoop();
  if (!loop.initialized()) return;
  PyEventLoop::Future future = loop.createFuture();

  // Callbacks to settle the Python asyncio.Future once the JS Promise is resolved
  JS::RootedObject onFulfilled = JS::RootedObject(cx, (JSObject *)js::NewFunctionWithReserved(cx, onResolvedCb, 1, 0, NULL));
  js::SetFunctionNativeReserved(onFulfilled, PY_FUTURE_OBJ_SLOT, JS::PrivateValue(future.getFutureObject())); // put the address of the Python object in private slot so we can access it later
  AddPromiseReactions(cx, promise, onFulfilled, nullptr);
  // TODO (Tom Tang): JS onRejected -> Py set_exception, maybe reuse `onFulfilled` and detect if the promise is rejected

  pyObject = future.getFutureObject(); // must be a new reference
}

void PromiseType::print(std::ostream &os) const {}
