/**
 * @file DateType.hh
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Struct for representing python dates
 * @version 0.1
 * @date 2022-12-21
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PythonMonkey_DateType_
#define PythonMonkey_DateType_

#include "PyType.hh"
#include "TypeEnum.hh"

#include <jsapi.h>
#include <js/Date.h>

#include <Python.h>

/**
 * @brief This struct represents the 'datetime' type in Python from the datetime module, which is represented as a 'Date' object in JS. It inherits from the PyType struct
 */
struct DateType : public PyType {
public:
  DateType(PyObject *object);
  /**
   * @brief Convert a JS Date object to Python datetime
   */
  DateType(JSContext *cx, JS::HandleObject dateObj);

  const TYPE returnType = TYPE::DATE;

  /**
   * @brief Convert a Python datetime object to JS Date
   *
   * @param cx - javascript context pointer
   */
  JSObject *toJsDate(JSContext *cx);
};

#endif