#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/setSpiderMonkeyException.hh"

#include "include/ExceptionType.hh"

#include <jsapi.h>
#include <js/Exception.h>

#include <Python.h>

ExceptionType::ExceptionType(PyObject *object) : PyType(object) {}

ExceptionType::ExceptionType(JSContext *cx, JS::HandleObject error) {
  // Convert the JS Error object to a Python string
  JS::RootedValue errValue(cx, JS::ObjectValue(*error)); // err
  JS::RootedObject errStack(cx, JS::ExceptionStackOrNull(error)); // err.stack
  PyObject *errStr = getExceptionString(cx, JS::ExceptionStack(cx, errValue, errStack));

  // Construct a new SpiderMonkeyError python object
  //    pyObject = SpiderMonkeyError(errStr)
  pyObject = PyObject_CallOneArg(SpiderMonkeyError, errStr); // _PyErr_CreateException, https://github.com/python/cpython/blob/3.9/Python/errors.c#L100
  Py_XDECREF(errStr);
}

void ExceptionType::print(std::ostream &os) const {}
