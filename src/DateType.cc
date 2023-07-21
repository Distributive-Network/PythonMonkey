#include "include/DateType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <jsapi.h>
#include <js/Date.h>

#include <Python.h>
#include <datetime.h>

// https://github.com/python/cpython/blob/v3.10.11/Modules/_datetimemodule.c#L89-L92
#define DATE_SET_MICROSECOND(o, v) \
  (((o)->data[7] = ((v) & 0xff0000) >> 16), \
  ((o)->data[8] = ((v) & 0x00ff00) >> 8), \
  ((o)->data[9] = ((v) & 0x0000ff)))

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
  Py_INCREF(PyDateTime_TimeZone_UTC); // PyTuple_SetItem steals the reference
  Py_DECREF(timestampArg);

  // Round to milliseconds precision because the smallest unit for a JS Date is 1ms
  double microseconds = PyDateTime_DATE_GET_MICROSECOND(pyObject);
  DATE_SET_MICROSECOND(
    (PyDateTime_DateTime *)pyObject,
    std::lround(microseconds / 1000) * 1000
  );
}

JSObject *DateType::toJsDate(JSContext *cx) {
  // See https://docs.python.org/3/library/datetime.html#datetime.datetime.timestamp
  PyObject *timestamp = PyObject_CallMethod(pyObject, "timestamp", NULL); // the result is in seconds
  double milliseconds = PyFloat_AsDouble(timestamp) * 1000;
  Py_DECREF(timestamp);
  return JS::NewDateObject(cx, JS::TimeClip(milliseconds));
}
