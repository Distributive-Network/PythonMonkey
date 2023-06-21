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
  #if PY_VERSION_HEX >= 0x03090000
  pyObject = PyObject_CallOneArg(SpiderMonkeyError, errStr); // _PyErr_CreateException, https://github.com/python/cpython/blob/3.9/Python/errors.c#L100
  #else
  pyObject = PyObject_CallFunction(SpiderMonkeyError, "O", errStr); // PyObject_CallOneArg is not available in Python < 3.9
  #endif
  Py_XDECREF(errStr);
}

// TODO (Tom Tang): preserve the original Python exception object somewhere in the JS obj for lossless two-way conversion
JSObject *ExceptionType::toJsError(JSContext *cx) {
  PyObject *pyErrType = PyObject_Type(pyObject);
  const char *pyErrTypeName = _PyType_Name((PyTypeObject *)pyErrType);
  PyObject *pyErrMsg = PyObject_Str(pyObject);
  // TODO (Tom Tang): Convert Python traceback and set it as the `stack` property on JS Error object
  // PyObject *traceback = PyException_GetTraceback(pyObject);

  std::stringstream msgStream;
  msgStream << "Python " << pyErrTypeName << ": " << PyUnicode_AsUTF8(pyErrMsg);
  std::string msg = msgStream.str();

  JS::RootedValue rval(cx);
  JS::RootedObject stack(cx);
  JS::RootedString filename(cx, JS_NewStringCopyZ(cx, "[python code]"));
  JS::RootedString message(cx, JS_NewStringCopyZ(cx, msg.c_str()));
  JS::CreateError(cx, JSExnType::JSEXN_ERR, stack, filename, 0, 0, nullptr, message, JS::NothingHandleValue, &rval);

  Py_DECREF(pyErrType);
  Py_DECREF(pyErrMsg);

  return rval.toObjectOrNull();
}
