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

#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include "include/PromiseType.hh"

#include "include/PyEventLoop.hh"
#include "include/PyType.hh"
#include "include/TypeEnum.hh"
#include "include/pyTypeFactory.hh"
#include "include/jsTypeFactory.hh"

#include <jsapi.h>
#include <jsfriendapi.h>
#include <js/Promise.h>

#include <Python.h>

#define PY_FUTURE_OBJ_SLOT 0 // slot id to access the python object in JS callbacks
#define PROMISE_OBJ_SLOT 1
// slot id must be less than 2 (https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/JSFunction.h#l866), otherwise it will access to arbitrary unsafe memory locations

static bool onResolvedCb(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Get the Promise state
  JS::Value promiseObjVal = js::GetFunctionNativeReserved(&args.callee(), PROMISE_OBJ_SLOT);
  JS::RootedObject promise = JS::RootedObject(cx, &promiseObjVal.toObject());
  JS::PromiseState state = JS::GetPromiseState(promise);

  // Convert the Promise's result (either fulfilled resolution or rejection reason) to a Python object
  // FIXME (Tom Tang): memory leak, not free-ed
  //  The result might be another JS function, so we must keep them alive
  JS::RootedObject *thisv = new JS::RootedObject(cx);
  args.computeThis(cx, thisv); // thisv is the global object, not the promise
  JS::RootedValue *resultArg = new JS::RootedValue(cx, args[0]);
  PyObject *result = pyTypeFactory(cx, thisv, resultArg)->getPyObject();
  if (state == JS::PromiseState::Rejected && !PyExceptionInstance_Check(result)) {
    // Wrap the result object into a SpiderMonkeyError object
    // because only *Exception objects can be thrown in Python `raise` statement and alike
    #if PY_VERSION_HEX >= 0x03090000
    PyObject *wrapped = PyObject_CallOneArg(SpiderMonkeyError, result); // wrapped = SpiderMonkeyError(result)
    #else
    PyObject *wrapped = PyObject_CallFunction(SpiderMonkeyError, "O", result); // PyObject_CallOneArg is not available in Python < 3.9
    #endif
    Py_XDECREF(result);
    result = wrapped;
  }

  // Get the `asyncio.Future` Python object from function's reserved slot
  JS::Value futureObjVal = js::GetFunctionNativeReserved(&args.callee(), PY_FUTURE_OBJ_SLOT);
  PyObject *futureObj = (PyObject *)(futureObjVal.toPrivate());

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

// Callback to resolve or reject the JS Promise when the Future is done
static PyObject *futureOnDoneCallback(PyObject *futureCallbackTuple, PyObject *args) {
  JSContext *cx = (JSContext *)PyLong_AsVoidPtr(PyTuple_GetItem(futureCallbackTuple, 0));
  auto rootedPtr = (JS::PersistentRooted<JSObject *> *)PyLong_AsVoidPtr(PyTuple_GetItem(futureCallbackTuple, 1));
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

JSObject *PromiseType::toJsPromise(JSContext *cx) {
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

  return promise;
}

bool PythonAwaitable_Check(PyObject *obj) {
  // see https://docs.python.org/3/c-api/typeobj.html#c.PyAsyncMethods
  PyTypeObject *tp = Py_TYPE(obj);
  bool isAwaitable = tp->tp_as_async != NULL && tp->tp_as_async->am_await != NULL;
  return isAwaitable;
}
