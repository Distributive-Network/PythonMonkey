/**
 * @file PyEventLoop.cc
 * @author Tom Tang (xmader@distributive.network)
 * @brief Send jobs to the Python event-loop
 * @date 2023-04-05
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */


#include "include/PyEventLoop.hh"

#include <Python.h>

/**
 * @brief Wrapper to decrement the counter of queueing event-loop jobs after the job finishes
 */
static PyObject *eventLoopJobWrapper(PyObject *jobFn, PyObject *Py_UNUSED(_)) {
  PyObject *ret = PyObject_CallObject(jobFn, NULL);
  Py_XDECREF(ret); // don't care about its return value

  PyObject *type, *value, *traceback;
  PyErr_Fetch(&type, &value, &traceback); // Protects `decCounter()`. If the error indicator is set, Python cannot make further function calls.
  PyEventLoop::_locker->decCounter();
  PyErr_Restore(type, value, traceback);

  if (PyErr_Occurred()) {
    return NULL;
  } else {
    Py_RETURN_NONE;
  }
}
static PyMethodDef loopJobWrapperDef = {"eventLoopJobWrapper", eventLoopJobWrapper, METH_NOARGS, NULL};

static PyObject *_enqueueWithDelay(PyObject *_loop, PyEventLoop::AsyncHandle::id_t handleId, PyObject *jobFn, double delaySeconds, bool repeat);

/**
 * @brief Wrapper to remove the reference of the timer after the job finishes
 */
static PyObject *timerJobWrapper(PyObject *jobFn, PyObject *args) {
  PyObject *_loop = PyTuple_GetItem(args, 0);
  PyEventLoop::AsyncHandle::id_t handleId = PyLong_AsLong(PyTuple_GetItem(args, 1));
  double delaySeconds = PyFloat_AsDouble(PyTuple_GetItem(args, 2));
  bool repeat = (bool)PyLong_AsLong(PyTuple_GetItem(args, 3));

  PyObject *ret = PyObject_CallObject(jobFn, NULL); // jobFn()
  Py_XDECREF(ret); // don't care about its return value

  PyObject *errType, *errValue, *traceback; // we can't call any Python code unless the error indicator is clear
  PyErr_Fetch(&errType, &errValue, &traceback);
  // Making sure a `AsyncHandle::fromId` call is close to its `handle`'s use.
  // We need to ensure the memory block doesn't move for reallocation before we can use the pointer,
  // as we could have multiple new `setTimeout` calls to expand the `_timeoutIdMap` vector while running the job function in parallel.
  auto handle = PyEventLoop::AsyncHandle::fromId(handleId);
  if (repeat && !handle->cancelled()) {
    _enqueueWithDelay(_loop, handleId, jobFn, delaySeconds, repeat);
  } else {
    handle->removeRef();
  }

  if (errType != NULL) { // PyErr_Occurred()
    PyErr_Restore(errType, errValue, traceback);
    return NULL;
  } else {
    Py_RETURN_NONE;
  }
}
static PyMethodDef timerJobWrapperDef = {"timerJobWrapper", timerJobWrapper, METH_VARARGS, NULL};

PyEventLoop::AsyncHandle PyEventLoop::enqueue(PyObject *jobFn) {
  PyEventLoop::_locker->incCounter();
  PyObject *wrapper = PyCFunction_New(&loopJobWrapperDef, jobFn);
  // Enqueue job to the Python event-loop
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.loop.call_soon
  PyObject *asyncHandle = PyObject_CallMethod(_loop, "call_soon_threadsafe", "O", wrapper);
  return PyEventLoop::AsyncHandle(asyncHandle);
}

static PyObject *_enqueueWithDelay(PyObject *_loop, PyEventLoop::AsyncHandle::id_t handleId, PyObject *jobFn, double delaySeconds, bool repeat) {
  PyObject *wrapper = PyCFunction_New(&timerJobWrapperDef, jobFn);
  // Schedule job to the Python event-loop
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.loop.call_later
  PyObject *asyncHandle = PyObject_CallMethod(_loop, "call_later", "dOOIdb", delaySeconds, wrapper, _loop, handleId, delaySeconds, repeat); // https://docs.python.org/3/c-api/arg.html#c.Py_BuildValue
  if (!asyncHandle) {
    return nullptr; // RuntimeError
  }

  auto handle = PyEventLoop::AsyncHandle::fromId(handleId);
  Py_XDECREF(handle->swap(asyncHandle));

  return asyncHandle;
}

PyEventLoop::AsyncHandle::id_t PyEventLoop::enqueueWithDelay(PyObject *jobFn, double delaySeconds, bool repeat) {
  auto handleId = PyEventLoop::AsyncHandle::newEmpty();
  if (!_enqueueWithDelay(_loop, handleId, jobFn, delaySeconds, repeat)) {
    PyErr_Print(); // RuntimeError: Non-thread-safe operation invoked on an event loop other than the current one
  }
  auto handle = PyEventLoop::AsyncHandle::fromId(handleId);
  handle->addRef();
  return handleId;
}

PyEventLoop::Future PyEventLoop::createFuture() {
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.loop.create_future
  PyObject *futureObj = PyObject_CallMethod(_loop, "create_future", NULL);
  return PyEventLoop::Future(futureObj);
}

PyEventLoop::Future PyEventLoop::ensureFuture(PyObject *awaitable) {
  PyObject *asyncio = PyImport_ImportModule("asyncio");

  PyObject *ensure_future_fn = PyObject_GetAttrString(asyncio, "ensure_future"); // ensure_future_fn = asyncio.ensure_future
  // instead of a simpler `PyObject_CallMethod`, only the `PyObject_Call` API function can be used here because `loop` is a keyword-only argument
  //    see https://docs.python.org/3.9/library/asyncio-future.html#asyncio.ensure_future
  //        https://docs.python.org/3/c-api/call.html#object-calling-api
  PyObject *args = PyTuple_New(1);
  PyTuple_SetItem(args, 0, awaitable);
  PyObject *kwargs = PyDict_New();
  PyDict_SetItemString(kwargs, "loop", _loop);
  PyObject *futureObj = PyObject_Call(ensure_future_fn, args, kwargs); // futureObj = ensure_future_fn(awaitable, loop=_loop)

  // clean up
  Py_DECREF(asyncio);
  Py_DECREF(ensure_future_fn);
  Py_DECREF(args);
  Py_DECREF(kwargs);

  Py_INCREF(futureObj); // needs to be kept alive as `PyEventLoop::Future` will decrease the reference count in its destructor
  return PyEventLoop::Future(futureObj);
}

/* static */
PyEventLoop PyEventLoop::_loopNotFound() {
  PyErr_SetString(PyExc_RuntimeError, "PythonMonkey cannot find a running Python event-loop to make asynchronous calls.");
  return PyEventLoop(nullptr);
}

/* static */
PyEventLoop PyEventLoop::_getLoopOnThread(PyThreadState *tstate) {
  // Modified from Python 3.9 `get_running_loop` https://github.com/python/cpython/blob/7cb3a44/Modules/_asynciomodule.c#L241-L278

  PyObject *ts_dict;
  #if PY_VERSION_HEX >= 0x030d0000 // Python version is greater than 3.13
  // The private `_PyThreadState_GetDict(tstate)` API gets removed in Python 3.13.
  // However, simply replacing it with `PyThreadState_GetDict()` does not work,
  //   since the public `PyThreadState_GetDict()` API can only get from the current thread.
  // We need to somehow get the thread dictionary on the main thread instead of the current thread.
  //
  // UPDATE: We don't need the thread dictionary anymore.
  // To get the thread's running event-loop in Python 3.13 is as simple as `thread_state->asyncio_running_loop`
  {
    // Every `PyThreadState` is actually allocated with extra fields as a `_PyThreadStateImpl` struct
    // See https://github.com/python/cpython/blob/v3.13.0rc1/Include/internal/pycore_tstate.h#L17-L24
    using PyThreadStateHolder = struct { // _PyThreadStateImpl
      PyThreadState base;
      #if PY_VERSION_HEX >= 0x030e0000 // Python version is greater than 3.14
      // the struct is changed with more additional fields, see https://github.com/python/cpython/blob/v3.14.0rc3/Include/internal/pycore_tstate.h#L24-L40
      Py_ssize_t refcount;
      uintptr_t c_stack_top;
      uintptr_t c_stack_soft_limit;
      uintptr_t c_stack_hard_limit;
      #endif
      PyObject *asyncio_running_loop; // we only need the first few fields of `_PyThreadStateImpl`
    };

    // Modified from https://github.com/python/cpython/blob/v3.13.0rc1/Modules/_asynciomodule.c#L3205-L3210
    PyObject *loop = ((PyThreadStateHolder *)tstate)->asyncio_running_loop;
    if (loop == NULL) {
      return _loopNotFound();
    }

    Py_INCREF(loop);
    return PyEventLoop(loop);
  }
  #elif PY_VERSION_HEX >= 0x03090000 // Python version is greater than 3.9
  ts_dict = _PyThreadState_GetDict(tstate);  // borrowed reference
  #else // Python 3.8
  ts_dict = tstate->dict; // see https://github.com/python/cpython/blob/v3.8.17/Modules/_asynciomodule.c#L244-L245
  #endif
  if (ts_dict == NULL) {
    return _loopNotFound();
  }

  // TODO: Python `get_running_loop` caches the PyRunningLoopHolder, should we do it as well for the main thread?
  //    see https://github.com/python/cpython/blob/7cb3a44/Modules/_asynciomodule.c#L234-L239
  PyObject *rl = PyDict_GetItemString(ts_dict, "__asyncio_running_event_loop__");  // borrowed reference
  if (rl == NULL) {
    return _loopNotFound();
  }

#if PY_VERSION_HEX < 0x030c0000 // Python version is less than 3.12
  using PyRunningLoopHolder = struct {
    PyObject_HEAD
    PyObject *rl_loop;
  };

  PyObject *running_loop = ((PyRunningLoopHolder *)rl)->rl_loop;
#else
  // The running loop is simply the `rl` object in Python 3.12+
  //    see https://github.com/python/cpython/blob/v3.12.0b2/Modules/_asynciomodule.c#L301
  PyObject *running_loop = rl;
#endif
  if (running_loop == Py_None) {
    return _loopNotFound();
  }

  Py_INCREF(running_loop);
  return PyEventLoop(running_loop);
}

/* static */
PyThreadState *PyEventLoop::_getMainThread() {
  // The last element in the linked-list of threads associated with the main interpreter should be the main thread
  // (The first element is the current thread, see https://github.com/python/cpython/blob/7cb3a44/Python/pystate.c#L291-L293)
  PyInterpreterState *interp = PyInterpreterState_Main();
  PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
  while (PyThreadState_Next(tstate) != nullptr) {
    tstate = PyThreadState_Next(tstate);
  }
  return tstate;
}

/* static inline */
PyThreadState *PyEventLoop::_getCurrentThread() {
  // `PyThreadState_Get` is used under the hood of the Python `asyncio.get_running_loop` method,
  // see https://github.com/python/cpython/blob/7cb3a44/Modules/_asynciomodule.c#L234
  return PyThreadState_Get();
}

/* static */
PyEventLoop PyEventLoop::getMainLoop() {
  return _getLoopOnThread(_getMainThread());
}

/* static */
PyEventLoop PyEventLoop::getRunningLoop() {
  return _getLoopOnThread(_getCurrentThread());
}

void PyEventLoop::AsyncHandle::cancel() {
  if (!_finishedOrCancelled()) {
    removeRef(); // automatically unref at finish
  }

  // https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.Handle.cancel
  PyObject *ret = PyObject_CallMethod(_handle, "cancel", NULL); // returns None
  Py_XDECREF(ret);
}

/* static */
bool PyEventLoop::AsyncHandle::cancelAll() {
  for (AsyncHandle &handle: _timeoutIdMap) {
    handle.cancel();
  }
  return true;
}

bool PyEventLoop::AsyncHandle::cancelled() {
  // https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.Handle.cancelled
  PyObject *ret = PyObject_CallMethod(_handle, "cancelled", NULL); // returns Python bool
  bool cancelled = ret == Py_True;
  Py_XDECREF(ret);
  return cancelled;
}

bool PyEventLoop::AsyncHandle::_finishedOrCancelled() {
  PyObject *scheduled = PyObject_GetAttrString(_handle, "_scheduled"); // this attribute only exists on asyncio.TimerHandle returned by loop.call_later
                                                                       // NULL if no such attribute (on a strict asyncio.Handle returned by loop.call_soon)
  bool notScheduled = scheduled && scheduled == Py_False; // not scheduled means the job function has already been executed or canceled
  Py_XDECREF(scheduled);
  return notScheduled;
}

void PyEventLoop::Future::setResult(PyObject *result) {
  // https://docs.python.org/3/library/asyncio-future.html#asyncio.Future.set_result
  PyObject *ret = PyObject_CallMethod(_future, "set_result", "O", result); // returns None
  Py_XDECREF(ret);
}

void PyEventLoop::Future::setException(PyObject *exception) {
  // https://docs.python.org/3/library/asyncio-future.html#asyncio.Future.set_exception
  PyObject *ret = PyObject_CallMethod(_future, "set_exception", "O", exception); // returns None
  Py_XDECREF(ret);
}

void PyEventLoop::Future::addDoneCallback(PyObject *cb) {
  // https://docs.python.org/3.9/library/asyncio-future.html#asyncio.Future.add_done_callback
  PyObject *ret = PyObject_CallMethod(_future, "add_done_callback", "O", cb); // returns None
  Py_XDECREF(ret);
}

bool PyEventLoop::Future::isCancelled() {
  // https://docs.python.org/3.9/library/asyncio-future.html#asyncio.Future.cancelled
  PyObject *ret = PyObject_CallMethod(_future, "cancelled", NULL); // returns Python bool
  bool cancelled = ret == Py_True;
  Py_XDECREF(ret);
  return cancelled;
}

PyObject *PyEventLoop::Future::getResult() {
  // https://docs.python.org/3.9/library/asyncio-future.html#asyncio.Future.result
  return PyObject_CallMethod(_future, "result", NULL);
}

PyObject *PyEventLoop::Future::getException() {
  // https://docs.python.org/3.9/library/asyncio-future.html#asyncio.Future.exception
  return PyObject_CallMethod(_future, "exception", NULL);
}
