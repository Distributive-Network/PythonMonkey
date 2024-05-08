/**
 * @file DateType.hh
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing python dates
 * @date 2022-12-21
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#ifndef PythonMonkey_DateType_
#define PythonMonkey_DateType_

#include <jsapi.h>
#include <js/Date.h>

#include <Python.h>

/**
 * @brief This struct represents the 'datetime' type in Python from the datetime module, which is represented as a 'Date' object in JS
 */
struct DateType {
public:
  /**
   * @brief Convert a JS Date object to Python datetime
   */
  static PyObject *getPyObject(JSContext *cx, JS::HandleObject dateObj);

  /**
   * @brief Convert a Python datetime object to JS Date
   *
   * @param cx - javascript context pointer
   * @param pyObject - the python datetime object to be converted
   */
  static JSObject *toJsDate(JSContext *cx, PyObject *pyObject);
};

#endif