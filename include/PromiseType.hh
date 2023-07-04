/**
 * @file PromiseType.hh
 * @author Tom Tang (xmader@distributive.network)
 * @brief Struct for representing Promises
 * @version 0.1
 * @date 2023-03-29
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef PythonMonkey_PromiseType_
#define PythonMonkey_PromiseType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <jsapi.h>
#include <js/Promise.h>

#include <Python.h>

/**
 * @brief This struct represents the JS Promise type in Python using our custom pythonmonkey.promise type. It inherits from the PyType struct
 */
struct PromiseType : public PyType {
public:
  PromiseType(PyObject *object);

  /**
   * @brief Construct a new PromiseType object from a JS::PromiseObject.
   *
   * @param cx - javascript context pointer
   * @param promise - JS::PromiseObject to be coerced
   */
  PromiseType(JSContext *cx, JS::HandleObject promise);

  const TYPE returnType = TYPE::PYTHONMONKEY_PROMISE;

  /**
   * @brief Convert a Python [awaitable](https://docs.python.org/3/library/asyncio-task.html#awaitables) object to JS Promise
   *
   * @param cx - javascript context pointer
   */
  JSObject *toJsPromise(JSContext *cx);
};

/**
 * @brief Check if the object can be used in Python await expression.
 * `PyAwaitable_Check` hasn't been and has no plan to be added to the Python C API as of CPython 3.9
 */
bool PythonAwaitable_Check(PyObject *obj);

/**
 * @brief Callback to resolve or reject the JS Promise when the Future is done
 * @see https://docs.python.org/3.9/library/asyncio-future.html#asyncio.Future.add_done_callback
 *
 * @param futureCallbackTuple - tuple( javascript context pointer, rooted JS Promise object )
 * @param args - Args tuple. The callback is called with the Future object as its only argument
 */
static PyObject *futureOnDoneCallback(PyObject *futureCallbackTuple, PyObject *args);

/**
 * @brief Callbacks to settle the Python asyncio.Future once the JS Promise is resolved
 */
static bool onResolvedCb(JSContext *cx, unsigned argc, JS::Value *vp);

#endif