#include "include/PyEventLoop.hh"

#include <Python.h>

/**
 * @brief Wrapper to decrement the counter of queueing event-loop jobs after the job finishes
 */
static PyObject *eventLoopJobWrapper(PyObject *jobFn, PyObject *Py_UNUSED(_)) {
  PyObject *ret = PyObject_CallObject(jobFn, NULL); // jobFn()
  Py_XDECREF(ret); // don't care about its return value
  PyEventLoop::_locker->decCounter();
  if (PyErr_Occurred()) {
    return NULL;
  } else {
    Py_RETURN_NONE;
  }
}
static PyMethodDef jobWrapperDef = {"eventLoopJobWrapper", eventLoopJobWrapper, METH_NOARGS, NULL};

PyEventLoop::AsyncHandle PyEventLoop::enqueue(PyObject *jobFn) {
  PyEventLoop::_locker->incCounter();
  PyObject *wrapper = PyCFunction_New(&jobWrapperDef, jobFn);
  // Enqueue job to the Python event-loop
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.loop.call_soon
  PyObject *asyncHandle = PyObject_CallMethod(_loop, "call_soon_threadsafe", "O", wrapper); // https://docs.python.org/3/c-api/arg.html#c.Py_BuildValue
  return PyEventLoop::AsyncHandle(asyncHandle);
}

PyEventLoop::AsyncHandle PyEventLoop::enqueueWithDelay(PyObject *jobFn, double delaySeconds) {
  PyEventLoop::_locker->incCounter();
  PyObject *wrapper = PyCFunction_New(&jobWrapperDef, jobFn);
  // Schedule job to the Python event-loop
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.loop.call_later
  PyObject *asyncHandle = PyObject_CallMethod(_loop, "call_later", "dO", delaySeconds, wrapper); // https://docs.python.org/3/c-api/arg.html#c.Py_BuildValue
  if (asyncHandle == nullptr) {
    PyErr_Print(); // RuntimeError: Non-thread-safe operation invoked on an event loop other than the current one
  }
  return PyEventLoop::AsyncHandle(asyncHandle);
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
  #if PY_VERSION_HEX >= 0x03090000 // Python version is greater than 3.9
  PyObject *ts_dict = _PyThreadState_GetDict(tstate);  // borrowed reference
  #else // Python 3.8
  PyObject *ts_dict = tstate->dict; // see https://github.com/python/cpython/blob/v3.8.17/Modules/_asynciomodule.c#L244-L245
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
  PyThreadState *tstate = PyInterpreterState_ThreadHead(interp); // https://docs.python.org/3/c-api/init.html#c.PyInterpreterState_ThreadHead
  while (PyThreadState_Next(tstate) != nullptr) {
    tstate = PyThreadState_Next(tstate);
  }
  return tstate;
}

/* static inline */
PyThreadState *PyEventLoop::_getCurrentThread() {
  // `PyThreadState_Get` is used under the hood of the Python `asyncio.get_running_loop` method,
  // see https://github.com/python/cpython/blob/7cb3a44/Modules/_asynciomodule.c#L234
  return PyThreadState_Get(); // https://docs.python.org/3/c-api/init.html#c.PyThreadState_Get
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
  PyObject *scheduled = PyObject_GetAttrString(_handle, "_scheduled"); // this attribute only exists on asyncio.TimerHandle returned by loop.call_later
                                                                       // NULL if no such attribute (on a strict asyncio.Handle returned by loop.call_soon)
  bool finishedOrCanceled = scheduled && scheduled == Py_False; // the job function has already been executed or canceled
  if (!finishedOrCanceled) {
    PyEventLoop::_locker->decCounter();
  }
  Py_XDECREF(scheduled);

  // https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.Handle.cancel
  PyObject *ret = PyObject_CallMethod(_handle, "cancel", NULL); // returns None
  Py_XDECREF(ret);
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
