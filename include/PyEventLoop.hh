/**
 * @file PyEventLoop.hh
 * @author Tom Tang (xmader@distributive.network)
 * @brief
 * @version 0.1
 * @date 2023-04-05
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef PythonMonkey_PyEventLoop_
#define PythonMonkey_PyEventLoop_

#include <Python.h>

struct PyEventLoop {
public:
  ~PyEventLoop() {
    Py_XDECREF(_loop);
  }

  bool initialized() const {
    return !!_loop;
  }

  /**
   * @brief Send job to the Python event-loop
   * @param jobFn - The JS event-loop job converted to a Python function
   * @return success
   */
  bool enqueue(PyObject *jobFn);
  bool enqueueWithDelay(PyObject *jobFn, double delaySeconds);

  /**
   * @brief Get the running Python event-loop, or
   *        raise a Python RuntimeError if no event-loop running
   * @return an instance of `PyEventLoop`
   */
  static PyEventLoop getRunningLoop();

protected:
  PyObject *_loop;

  PyEventLoop() = delete;
  PyEventLoop(PyObject *loop) : _loop(loop) {};
};

#endif