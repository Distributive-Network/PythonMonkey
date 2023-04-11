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
#include "include/PyEventLoop.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"
#include "include/pyTypeFactory.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Promise.h>

#include <Python.h>

#define PY_FUTURE_OBJ_SLOT 20 // (arbitrarily chosen) slot id to access the python object in JS callbacks
#define PROMISE_OBJ_SLOT 21

static bool onResolvedCb(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Convert the Promise's result (either fulfilled resolution or rejection reason) to a Python object
  JS::RootedObject thisv(cx);
  // args.computeThis(cx, &thisv); // thisv is the global object, not the promise
  JS::RootedValue resultArg(cx, args[0]);
  PyObject *result = pyTypeFactory(cx, &thisv, &resultArg)->getPyObject();

  // Get the `asyncio.Future` Python object from function's reserved slot
  JS::Value futureObjVal = js::GetFunctionNativeReserved(&args.callee(), PY_FUTURE_OBJ_SLOT);
  PyObject *futureObj = (PyObject *)(futureObjVal.toPrivate());

  // Get the Promise state
  JS::Value promiseObjVal = js::GetFunctionNativeReserved(&args.callee(), PROMISE_OBJ_SLOT);
  JS::RootedObject promise = JS::RootedObject(cx, &promiseObjVal.toObject());
  JS::PromiseState state = JS::GetPromiseState(promise);

  // Settle the Python asyncio.Future by the Promise's result
  PyEventLoop::Future future = PyEventLoop::Future(futureObj);
  if (state == JS::PromiseState::Fulfilled) {
    future.setResult(result);
  } else { // state == JS::PromiseState::Rejected
    future.setException(result);
  }

  return true;
}

PromiseType::PromiseType(PyObject *object) : PyType(object) {}

PromiseType::PromiseType(JSContext *cx, JS::HandleObject promise) {
  // Create a python asyncio.Future on the running python event-loop
  PyEventLoop loop = PyEventLoop::getRunningLoop();
  if (!loop.initialized()) return;
  PyEventLoop::Future future = loop.createFuture();

  // Callbacks to settle the Python asyncio.Future once the JS Promise is resolved
  JS::RootedObject onResolved = JS::RootedObject(cx, (JSObject *)js::NewFunctionWithReserved(cx, onResolvedCb, 1, 0, NULL));
  js::SetFunctionNativeReserved(onResolved, PY_FUTURE_OBJ_SLOT, JS::PrivateValue(future.getFutureObject())); // put the address of the Python object in private slot so we can access it later
  js::SetFunctionNativeReserved(onResolved, PROMISE_OBJ_SLOT, JS::ObjectValue(*promise));
  AddPromiseReactions(cx, promise, onResolved, onResolved);

  pyObject = future.getFutureObject(); // must be a new reference
}

void PromiseType::print(std::ostream &os) const {}
