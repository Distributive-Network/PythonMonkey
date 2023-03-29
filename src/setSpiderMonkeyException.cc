/**
 * @file setSpiderMonkeyException.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief Call this function whenever a JS_* function call fails in order to set an appropriate python exception (remember to also return NULL)
 * @version 0.1
 * @date 2023-02-28
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/setSpiderMonkeyException.hh"
#include "include/StrType.hh"

#include <jsapi.h>

#include <codecvt>
#include <locale>

#include <Python.h>

void setSpiderMonkeyException(JSContext *cx) {
  if (!JS_IsExceptionPending(cx)) {
    PyErr_SetString(SpiderMonkeyError, "Spidermonkey failed, but spidermonkey did not set an exception.");
    return;
  }
  JS::ExceptionStack exceptionStack(cx);
  if (!JS::GetPendingExceptionStack(cx, &exceptionStack)) {
    PyErr_SetString(SpiderMonkeyError, "Spidermonkey set an exception, but was unable to retrieve it.");
  }
  JS::RootedObject exceptionObject(cx);
  if (!JS_ValueToObject(cx, exceptionStack.exception(), &exceptionObject)) {
    PyErr_SetString(SpiderMonkeyError, "Spidermonkey set an exception, but the exception could not be converted to an object.");
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

  JSErrorReport *errorReport = JS_ErrorFromException(cx, exceptionObject);
  std::stringstream outStrStream;

  std::string offsetSpaces(errorReport->tokenOffset(), ' '); // number of spaces equal to tokenOffset
  std::string linebuf; // the offending JS line of code (can be empty)

  outStrStream << "Error in file " << errorReport->filename << ", on line " << errorReport->lineno << ":\n";
  if (errorReport->linebuf()) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::u16string u16linebuf(errorReport->linebuf());
    linebuf = convert.to_bytes(u16linebuf);
  }
  if (linebuf.size()) {
    outStrStream << linebuf << "\n";
    outStrStream << offsetSpaces << "^\n";
  }
  outStrStream << errorReport->message().c_str() << "\n";

  if (exceptionStack.stack()) {
    JS::Rooted<JS::ValueArray<0>> args(cx);
    JS::RootedValue stackString(cx);
    JS_CallFunctionName(cx, exceptionStack.stack(), "toString", args, &stackString);
    StrType stackStr(cx, stackString.toString());
    outStrStream << "Stack Trace: \n" << stackStr.getValue();
  }

  PyErr_SetString(SpiderMonkeyError, outStrStream.str().c_str());
}