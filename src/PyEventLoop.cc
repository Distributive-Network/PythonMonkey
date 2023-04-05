#include "include/PyEventLoop.hh"

#include <Python.h>

bool PyEventLoop::enqueue(PyObject *jobFn) {
  // Enqueue job to the Python event-loop
  //    https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.loop.call_soon
  PyObject *asyncHandle = PyObject_CallMethod(_loop, "call_soon", "O", jobFn); // https://docs.python.org/3/c-api/arg.html#other-objects
  Py_DECREF(asyncHandle); // clean up
  return true;
}

bool PyEventLoop::enqueueWithDelay(PyObject *jobFn, double delaySeconds) {

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
