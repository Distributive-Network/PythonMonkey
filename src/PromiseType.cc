/**
 * @file PromiseType.cc
 * @author Tom Tang (xmader@distributive.network)
 * @brief Struct for representing Promises
 * @date 2023-03-29
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/PromiseType.hh"
#include "include/DictType.hh"
#include "include/PyEventLoop.hh"
#include "include/pyTypeFactory.hh"
#include "include/jsTypeFactory.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Promise.h>

// slot ids to access the python object in JS callbacks
#define PY_FUTURE_OBJ_SLOT 0
#define PROMISE_OBJ_SLOT 1

static bool onResolvedCb(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Get the Promise state
  JS::Value promiseObjVal = js::GetFunctionNativeReserved(&args.callee(), PROMISE_OBJ_SLOT);
  JS::RootedObject promise(cx, &promiseObjVal.toObject());
  JS::PromiseState state = JS::GetPromiseState(promise);

  // Convert the Promise's result (either fulfilled resolution or rejection reason) to a Python object
  //  The result might be another JS function, so we must keep them alive
  JS::RootedValue resultArg(cx, args[0]);
  PyObject *result = pyTypeFactory(cx, resultArg);
  if (state == JS::PromiseState::Rejected && !PyExceptionInstance_Check(result)) {
    // Wrap the result object into a SpiderMonkeyError object
    // because only *Exception objects can be thrown in Python `raise` statement and alike
    #if PY_VERSION_HEX >= 0x03090000
    PyObject *wrapped = PyObject_CallOneArg(SpiderMonkeyError, result); // wrapped = SpiderMonkeyError(result)
    #else
    PyObject *wrapped = PyObject_CallFunction(SpiderMonkeyError, "O", result); // PyObject_CallOneArg is not available in Python < 3.9
    #endif
    // Preserve the original JS value as the `jsError` attribute for lossless conversion back
    PyObject *originalJsErrCapsule = DictType::getPyObject(cx, resultArg);
    PyObject_SetAttrString(wrapped, "jsError", originalJsErrCapsule);
    Py_DECREF(result);
    result = wrapped;
  }

  // Get the `asyncio.Future` Python object from function's reserved slot
  JS::Value futureObjVal = js::GetFunctionNativeReserved(&args.callee(), PY_FUTURE_OBJ_SLOT);
  PyObject *futureObj = (PyObject *)(futureObjVal.toPrivate());

  // Settle the Python asyncio.Future by the Promise's result
  PyEventLoop::Future future = PyEventLoop::Future(futureObj); // will decrease the reference count of `futureObj` in its destructor when the `onResolvedCb` function ends
  if (state == JS::PromiseState::Fulfilled) {
    future.setResult(result);
  } else { // state == JS::PromiseState::Rejected
    future.setException(result);
  }

  Py_DECREF(result);
  // Py_DECREF(futureObj) // the destructor for the `PyEventLoop::Future` above already does this
  return true;
}

PyObject *PromiseType::getPyObject(JSContext *cx, JS::HandleObject promise) {
  // Create a python asyncio.Future on the running python event-loop
  PyEventLoop loop = PyEventLoop::getRunningLoop();
  if (!loop.initialized()) return NULL;
  PyEventLoop::Future future = loop.createFuture(); // ref count == 1

  // Callbacks to settle the Python asyncio.Future once the JS Promise is resolved
  JS::RootedObject onResolved = JS::RootedObject(cx, (JSObject *)js::NewFunctionWithReserved(cx, onResolvedCb, 1, 0, NULL));
  js::SetFunctionNativeReserved(onResolved, PY_FUTURE_OBJ_SLOT, JS::PrivateValue(future.getFutureObject())); // ref count == 2
  js::SetFunctionNativeReserved(onResolved, PROMISE_OBJ_SLOT, JS::ObjectValue(*promise));
  JS::AddPromiseReactions(cx, promise, onResolved, onResolved);

  return future.getFutureObject(); // must be a new reference, ref count == 3
  // Here the ref count for the `future` object is 3, but will immediately decrease to 2 in `PyEventLoop::Future`'s destructor when the `PromiseType::getPyObject` function ends
  // Leaving one reference for the returned Python object, and another one for the `onResolved` callback function
}

// Callback to resolve or reject the JS Promise when the Future is done
static PyObject *futureOnDoneCallback(PyObject *futureCallbackTuple, PyObject *args) {
  JSContext *cx = (JSContext *)PyLong_AsVoidPtr(PyTuple_GetItem(futureCallbackTuple, 0));
  JS::PersistentRootedObject *rootedPtr = (JS::PersistentRootedObject *)PyLong_AsVoidPtr(PyTuple_GetItem(futureCallbackTuple, 1));
  JS::HandleObject promise = *rootedPtr;
  PyObject *futureObj = PyTuple_GetItem(args, 0); // the callback is called with the Future object as its only argument
                                                  // see https://docs.python.org/3.9/library/asyncio-future.html#asyncio.Future.add_done_callback
  PyEventLoop::Future future = PyEventLoop::Future(futureObj);

  PyEventLoop::_locker->decCounter();

  PyObject *exception = future.getException();
  if (exception == NULL || PyErr_Occurred()) { // awaitable is cancelled, `futureObj.exception()` raises a CancelledError
    // Reject the promise with the CancelledError, or very unlikely, an InvalidStateError exception if the Future isn’t done yet
    //    see https://docs.python.org/3.9/library/asyncio-future.html#asyncio.Future.exception
    PyObject *errType, *errValue, *traceback;
    PyErr_Fetch(&errType, &errValue, &traceback); // also clears the Python error stack
    JS::RejectPromise(cx, promise, JS::RootedValue(cx, jsTypeFactorySafe(cx, errValue)));
    Py_XDECREF(errType); Py_XDECREF(errValue); Py_XDECREF(traceback);
  } else if (exception == Py_None) { // no exception set on this awaitable, safe to get result, otherwise the exception will be raised when calling `futureObj.result()`
    PyObject *result = future.getResult();
    JS::ResolvePromise(cx, promise, JS::RootedValue(cx, jsTypeFactorySafe(cx, result)));
    Py_DECREF(result);
  } else { // having exception set, to reject the promise
    JS::RejectPromise(cx, promise, JS::RootedValue(cx, jsTypeFactorySafe(cx, exception)));
  }
  Py_XDECREF(exception); // cleanup

  delete rootedPtr; // no longer needed to be rooted, clean it up

  // Py_DECREF(futureObj) // would cause bug, because `Future` constructor didn't increase futureObj's ref count, but the destructor will decrease it
  Py_RETURN_NONE;
}
static PyMethodDef futureCallbackDef = {"futureOnDoneCallback", futureOnDoneCallback, METH_VARARGS, NULL};

JSObject *PromiseType::toJsPromise(JSContext *cx, PyObject *pyObject) {
  // Create a new JS Promise object
  JSObject *promise = JS::NewPromiseObject(cx, nullptr);

  // Convert the python awaitable to an asyncio.Future object
  PyEventLoop loop = PyEventLoop::getRunningLoop();
  if (!loop.initialized()) return nullptr;
  PyEventLoop::Future future = loop.ensureFuture(pyObject);

  PyEventLoop::_locker->incCounter();

  // Resolve or Reject the JS Promise once the python awaitable is done
  JS::PersistentRooted<JSObject *> *rootedPtr = new JS::PersistentRooted<JSObject *>(cx, promise); // `promise` is required to be rooted from here to the end of onDoneCallback
  PyObject *futureCallbackTuple = PyTuple_Pack(2, PyLong_FromVoidPtr(cx), PyLong_FromVoidPtr(rootedPtr));
  PyObject *onDoneCb = PyCFunction_New(&futureCallbackDef, futureCallbackTuple);
  future.addDoneCallback(onDoneCb);
  Py_INCREF(pyObject);
  return promise;
}

bool PythonAwaitable_Check(PyObject *obj) {
  // see https://docs.python.org/3/c-api/typeobj.html#c.PyAsyncMethods
  PyTypeObject *tp = Py_TYPE(obj);
  bool isAwaitable = tp->tp_as_async != NULL && tp->tp_as_async->am_await != NULL;
  return isAwaitable;
}