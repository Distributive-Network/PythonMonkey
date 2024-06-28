/**
 * @file setSpiderMonkeyException.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Call this function whenever a JS_* function call fails in order to set an appropriate python exception (remember to also return NULL)
 * @date 2023-02-28
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/setSpiderMonkeyException.hh"
#include "include/StrType.hh"
#include "include/DictType.hh"

#include <jsapi.h>
#include <Python.h>

#include <codecvt>
#include <locale>

PyObject *getExceptionString(JSContext *cx, const JS::ExceptionStack &exceptionStack, bool printStack) {
  JS::ErrorReportBuilder reportBuilder(cx);
  if (!reportBuilder.init(cx, exceptionStack, JS::ErrorReportBuilder::WithSideEffects /* may call the `toString` method if an object is thrown */)) {
    return PyUnicode_FromString("Spidermonkey set an exception, but could not initialize the error report.");
  }

  /**
   * the resulting python error string will be in the format:
   * "Error in file <filename>, on line <lineno>:
   * <offending line of code if relevant, nothing if not>
   * <if offending line is present, then a '^' pointing to the relevant token>
   * <spidermonkey error message>
   * Stack Track:
   * <stack trace>"
   *
   */
  std::stringstream outStrStream;

  JSErrorReport *errorReport = reportBuilder.report();
  if (errorReport && !!errorReport->filename) { // `errorReport->filename` (the source file name) can be null
    std::string offsetSpaces(errorReport->tokenOffset(), ' '); // number of spaces equal to tokenOffset
    std::string linebuf; // the offending JS line of code (can be empty)

    /* *INDENT-OFF* */
    outStrStream << "Error in file " << errorReport->filename.c_str()
                 << ", on line " << errorReport->lineno
                 << ", column " << errorReport->column.oneOriginValue() << ":\n";
    /* *INDENT-ON* */
    if (errorReport->linebuf()) {
      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
      std::u16string u16linebuf(errorReport->linebuf());
      linebuf = convert.to_bytes(u16linebuf);
    }
    if (linebuf.size()) {
      outStrStream << linebuf << "\n";
      outStrStream << offsetSpaces << "^\n";
    }
  }

  // print out the SpiderMonkey error message
  outStrStream << reportBuilder.toStringResult().c_str() << "\n";

  if (printStack) {
    JS::RootedObject stackObj(cx, exceptionStack.stack());
    if (stackObj.get()) {
      JS::RootedString stackStr(cx);
      BuildStackString(cx, nullptr, stackObj, &stackStr, 2, js::StackFormat::SpiderMonkey);
      JS::RootedValue stackStrVal(cx, JS::StringValue(stackStr));
      outStrStream << "Stack Trace:\n" << StrType::getValue(cx, stackStrVal);
    }
  }

  return PyUnicode_FromString(outStrStream.str().c_str());
}

void setSpiderMonkeyException(JSContext *cx) {
  if (PyErr_Occurred()) { // Check if a Python exception has already been set, otherwise `PyErr_SetString` would overwrite the exception set
    return;
  }
  if (!JS_IsExceptionPending(cx)) {
    PyErr_SetString(SpiderMonkeyError, "Spidermonkey failed, but spidermonkey did not set an exception.");
    return;
  }
  JS::ExceptionStack exceptionStack(cx);
  if (!JS::GetPendingExceptionStack(cx, &exceptionStack)) {
    PyErr_SetString(SpiderMonkeyError, "Spidermonkey set an exception, but was unable to retrieve it.");
    return;
  }

  // check if it is a Python Exception and already has a stack trace
  bool printStack = true;

  JS::RootedValue exn(cx);
  if (JS_GetPendingException(cx, &exn)) {
    if (exn.isObject()) {
      JS::RootedObject exnObj(cx, &exn.toObject());
      JS::RootedValue tmp(cx);
      if (JS_GetProperty(cx, exnObj, "message", &tmp) && tmp.isString()) {
        JS::RootedString rootedStr(cx, tmp.toString());
        printStack = strstr(JS_EncodeStringToUTF8(cx, rootedStr).get(), "JS Stack Trace") == NULL;
      }
    }
  }

  JS_ClearPendingException(cx);

  // `PyErr_SetString` uses `PyErr_SetObject` with `PyUnicode_FromString` under the hood
  //    see https://github.com/python/cpython/blob/3.9/Python/errors.c#L234-L236
  PyObject *errStr = getExceptionString(cx, exceptionStack, printStack);
  PyObject *errObj = PyObject_CallFunction(SpiderMonkeyError, "O", errStr); // errObj = SpiderMonkeyError(errStr)
  Py_XDECREF(errStr);
  // Preserve the original JS value as the `jsError` attribute for lossless back conversion
  PyObject *originalJsErrCapsule = DictType::getPyObject(cx, exn);
  PyObject_SetAttrString(errObj, "jsError", originalJsErrCapsule);
  Py_XDECREF(originalJsErrCapsule);
  // `PyErr_SetObject` can accept either an already created Exception instance or the containing exception value as the second argument
  //  see https://github.com/python/cpython/blob/v3.9.16/Python/errors.c#L134-L150
  PyErr_SetObject(SpiderMonkeyError, errObj);
  Py_XDECREF(errObj);
}
