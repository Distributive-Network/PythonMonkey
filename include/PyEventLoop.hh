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
   * @brief C++ wrapper for Python `asyncio.Handle` class
   * @see https://docs.python.org/3/library/asyncio-eventloop.html#asyncio.Handle
   */
  struct AsyncHandle {
  public:
    AsyncHandle(PyObject *handle) : _handle(handle) {};
    ~AsyncHandle() {
      Py_XDECREF(_handle);
    }

    /**
     * @brief Cancel the scheduled event-loop job.
     * If the job has already been canceled or executed, this method has no effect.
     */
    void cancel();

    /**
     * @brief Get the unique `timeoutID` for JS `setTimeout`/`clearTimeout` methods
     * @see https://developer.mozilla.org/en-US/docs/Web/API/setTimeout#return_value
     */
    inline uint64_t getId() {
      // Currently we use the address of the underlying `asyncio.Handle` object
      // FIXME (Tom Tang): JS stores the `timeoutID` in a `number` (float64), may overflow
      return (uint64_t)_handle;
    }
    static inline AsyncHandle fromId(uint64_t timeoutID) {
      return AsyncHandle((PyObject *)timeoutID);
    }
  protected:
    PyObject *_handle;
  };

  /**
   * @brief Send job to the Python event-loop
   * @param jobFn - The JS event-loop job converted to a Python function
   * @return a AsyncHandle, the value can be safely ignored
   */
  AsyncHandle enqueue(PyObject *jobFn);
  /**
   * @brief Schedule a job to the Python event-loop, with the given delay
   * @param jobFn - The JS event-loop job converted to a Python function
   * @param delaySeconds - The job function will be called after the given number of seconds
   * @return a AsyncHandle, the value can be safely ignored
   */
  AsyncHandle enqueueWithDelay(PyObject *jobFn, double delaySeconds);

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