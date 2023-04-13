#include "include/PyEventLoop.hh"

#include <Python.h>

PyEventLoop::AsyncHandle PyEventLoop::enqueue(PyObject *jobFn) {
  // Enqueue job to the Python event-loop
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.loop.call_soon
  PyObject *asyncHandle = PyObject_CallMethod(_loop, "call_soon", "O", jobFn); // https://docs.python.org/3/c-api/arg.html#c.Py_BuildValue
  return PyEventLoop::AsyncHandle(asyncHandle);
}

PyEventLoop::AsyncHandle PyEventLoop::enqueueWithDelay(PyObject *jobFn, double delaySeconds) {
  // Schedule job to the Python event-loop
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.loop.call_later
  PyObject *asyncHandle = PyObject_CallMethod(_loop, "call_later", "dO", delaySeconds, jobFn); // https://docs.python.org/3/c-api/arg.html#c.Py_BuildValue
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
PyEventLoop PyEventLoop::getRunningLoop() {
  // Get the running Python event-loop
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.get_running_loop
  PyObject *asyncio = PyImport_ImportModule("asyncio");
  PyObject *loop = PyObject_CallMethod(asyncio, "get_running_loop", NULL);

  Py_DECREF(asyncio); // clean up

  if (loop == nullptr) { // `get_running_loop` would raise a RuntimeError if there is no running event loop
    // Overwrite the error raised by `get_running_loop`
    PyErr_SetString(PyExc_RuntimeError, "PythonMonkey cannot find a running Python event-loop to make asynchronous calls.");
  }

  return PyEventLoop(loop); // `loop` may be null
}

void PyEventLoop::AsyncHandle::cancel() {
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

PyObject *PyEventLoop::Future::getResult() {
  // https://docs.python.org/3.9/library/asyncio-future.html#asyncio.Future.result
  return PyObject_CallMethod(_future, "result", NULL);
}

PyObject *PyEventLoop::Future::getException() {
  // https://docs.python.org/3.9/library/asyncio-future.html#asyncio.Future.exception
  return PyObject_CallMethod(_future, "exception", NULL);
}
