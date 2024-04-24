/**
 * @file DateType.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing python dates
 * @date 2022-12-21
 *
 * @copyright Copyright (c) 2022,2024 Distributive Corp.
 *
 */

#include "include/DateType.hh"

#include <jsapi.h>
#include <js/Date.h>

#include <datetime.h>

PyObject *DateType::getPyObject(JSContext *cx, JS::HandleObject dateObj) {
  if (!PyDateTimeAPI) { PyDateTime_IMPORT; } // for PyDateTime_FromTimestamp

  JS::Rooted<JS::ValueArray<0>> args(cx);
  JS::Rooted<JS::Value> year(cx);
  JS::Rooted<JS::Value> month(cx);
  JS::Rooted<JS::Value> day(cx);
  JS::Rooted<JS::Value> hour(cx);
  JS::Rooted<JS::Value> minute(cx);
  JS::Rooted<JS::Value> second(cx);
  JS::Rooted<JS::Value> usecond(cx);
  JS_CallFunctionName(cx, dateObj, "getUTCFullYear", args, &year);
  JS_CallFunctionName(cx, dateObj, "getUTCMonth", args, &month);
  JS_CallFunctionName(cx, dateObj, "getUTCDate", args, &day);
  JS_CallFunctionName(cx, dateObj, "getUTCHours", args, &hour);
  JS_CallFunctionName(cx, dateObj, "getUTCMinutes", args, &minute);
  JS_CallFunctionName(cx, dateObj, "getUTCSeconds", args, &second);
  JS_CallFunctionName(cx, dateObj, "getUTCMilliseconds", args, &usecond);

  PyObject *pyObject = PyDateTimeAPI->DateTime_FromDateAndTime(
    year.toNumber(), month.toNumber() + 1, day.toNumber(),
    hour.toNumber(), minute.toNumber(), second.toNumber(),
    usecond.toNumber() * 1000,
    PyDateTime_TimeZone_UTC, // Make the resulting Python datetime object timezone-aware
                             // See https://docs.python.org/3/library/datetime.html#aware-and-naive-objects
    PyDateTimeAPI->DateTimeType
  );
  Py_INCREF(PyDateTime_TimeZone_UTC);

  return pyObject;
}

JSObject *DateType::toJsDate(JSContext *cx, PyObject *pyObject) {
  // See https://docs.python.org/3/library/datetime.html#datetime.datetime.timestamp
  PyObject *timestamp = PyObject_CallMethod(pyObject, "timestamp", NULL); // the result is in seconds
  double milliseconds = PyFloat_AsDouble(timestamp) * 1000;
  Py_DECREF(timestamp);
  return JS::NewDateObject(cx, JS::TimeClip(milliseconds));
}