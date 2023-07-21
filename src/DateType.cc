#include "include/DateType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <jsapi.h>
#include <js/Date.h>

#include <Python.h>
#include <datetime.h>

DateType::DateType(PyObject *object) : PyType(object) {}

DateType::DateType(JSContext *cx, JS::HandleObject dateObj) {
  if (!PyDateTimeAPI) { PyDateTime_IMPORT; } // for PyDateTime_FromTimestamp

  // Convert by the timestamp value
  JS::Rooted<JS::ValueArray<0>> args(cx);
  JS::Rooted<JS::Value> timeValue(cx);
  JS_CallFunctionName(cx, dateObj, "getTime", args, &timeValue);
  double milliseconds = timeValue.toNumber();

  PyObject *timestampArg = PyTuple_New(2);
  PyTuple_SetItem(timestampArg, 0, PyFloat_FromDouble(milliseconds / 1000));
  PyTuple_SetItem(timestampArg, 1, PyDateTime_TimeZone_UTC); // Make the resulting Python datetime object timezone-aware
                                                             // See https://docs.python.org/3/library/datetime.html#aware-and-naive-objects
  pyObject = PyDateTime_FromTimestamp(timestampArg);
  Py_DECREF(timestampArg);
}

JSObject *DateType::toJsDate(JSContext *cx) {
  // See https://docs.python.org/3/library/datetime.html#datetime.datetime.timestamp
  PyObject *timestamp = PyObject_CallMethod(pyObject, "timestamp", NULL); // the result is in seconds
  double milliseconds = PyFloat_AsDouble(timestamp) * 1000;
  Py_DECREF(timestamp);
  return JS::NewDateObject(cx, JS::TimeClip(milliseconds));
}
