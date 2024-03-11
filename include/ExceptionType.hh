/**
 * @file ExceptionType.hh
 * @author Tom Tang (xmader@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing Python Exception objects from a corresponding JS Error object
 * @version 0.1
 * @date 2023-04-11
 *
 * @copyright Copyright (c) 2023-2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_ExceptionType_
#define PythonMonkey_ExceptionType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <jsapi.h>

#include <Python.h>

/**
 * @brief This struct represents a Python Exception object from the corresponding JS Error object
 */
struct ExceptionType : public PyType {
public:
  /**
   * @brief Construct a new SpiderMonkeyError from the JS Error object.
   *
   * @param cx - javascript context pointer
   * @param error - JS Error object to be converted
   */
  ExceptionType(JSContext *cx, JS::HandleObject error);

  const TYPE returnType = TYPE::EXCEPTION;

  /**
   * @brief Convert a python Exception object to a JS Error object
   *
   * @param cx - javascript context pointer
   * @param exceptionValue - Exception object pointer, cannot be NULL
   * @param traceBack - Exception traceback pointer, can be NULL
   */
  static JSObject *toJsError(JSContext *cx, PyObject *exceptionValue, PyObject *traceBack);
};

#endif