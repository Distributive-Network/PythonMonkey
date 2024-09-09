/**
 * @file ExceptionType.cc
 * @author Tom Tang (xmader@distributive.network) and Philippe Laporte (philippe@distributive.network)
 * @brief Struct for representing Python Exception objects from a corresponding JS Error object
 * @date 2023-04-11
 *
 * @copyright Copyright (c) 2023-2024 Distributive Corp.
 *
 */

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/setSpiderMonkeyException.hh"

#include "include/ExceptionType.hh"
#include "include/StrType.hh"
#include "include/DictType.hh"
#include "include/JSObjectProxy.hh"

#include <jsapi.h>
#include <js/Exception.h>

#include <frameobject.h>


PyObject *ExceptionType::getPyObject(JSContext *cx, JS::HandleObject error) {
  // Convert the JS Error object to a Python string
  JS::RootedValue errValue(cx, JS::ObjectValue(*error)); // err
  JS::RootedObject errStack(cx, JS::ExceptionStackOrNull(error)); // err.stack
  PyObject *errStr = getExceptionString(cx, JS::ExceptionStack(cx, errValue, errStack), true);

  // Construct a new SpiderMonkeyError python object
  #if PY_VERSION_HEX >= 0x03090000
  PyObject *pyObject = PyObject_CallOneArg(SpiderMonkeyError, errStr); // _PyErr_CreateException, https://github.com/python/cpython/blob/3.9/Python/errors.c#L100
  #else
  PyObject *pyObject = PyObject_CallFunction(SpiderMonkeyError, "O", errStr); // PyObject_CallOneArg is not available in Python < 3.9
  #endif
  Py_XDECREF(errStr);

  // Preserve the original JS Error object as the Python Exception's `jsError` attribute for lossless two-way conversion
  PyObject *originalJsErrCapsule = DictType::getPyObject(cx, errValue);
  PyObject_SetAttrString(pyObject, "jsError", originalJsErrCapsule);

  return pyObject;
}


// Generating trace information

#define PyTraceBack_LIMIT 1000

static const int TB_RECURSIVE_CUTOFF = 3;

#if PY_VERSION_HEX >= 0x03090000

static inline int
tb_get_lineno(PyTracebackObject *tb) {
  PyFrameObject *frame = tb->tb_frame;
  PyCodeObject *code = PyFrame_GetCode(frame);
  int lineno = PyCode_Addr2Line(code, tb->tb_lasti);
  Py_DECREF(code);
  return lineno;
}

#endif

static int
tb_print_line_repeated(_PyUnicodeWriter *writer, long cnt)
{
  cnt -= TB_RECURSIVE_CUTOFF;
  PyObject *line = PyUnicode_FromFormat(
    (cnt > 1)
          ? "[Previous line repeated %ld more times]\n"
          : "[Previous line repeated %ld more time]\n",
    cnt);
  if (line == NULL) {
    return -1;
  }
  int err = _PyUnicodeWriter_WriteStr(writer, line);
  Py_DECREF(line);
  return err;
}

JSObject *ExceptionType::toJsError(JSContext *cx, PyObject *exceptionValue, PyObject *traceBack) {
  assert(exceptionValue != NULL);

  if (PyObject_HasAttrString(exceptionValue, "jsError")) {
    PyObject *originalJsErrCapsule = PyObject_GetAttrString(exceptionValue, "jsError");
    if (originalJsErrCapsule && PyObject_TypeCheck(originalJsErrCapsule, &JSObjectProxyType)) {
      return *((JSObjectProxy *)originalJsErrCapsule)->jsObject;
    }
  }

  // Gather JS context
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wformat-zero-length"
  JS_ReportErrorASCII(cx, ""); // throw JS error and gather all details
  #pragma GCC diagnostic pop

  JS::ExceptionStack exceptionStack(cx);
  if (!JS::GetPendingExceptionStack(cx, &exceptionStack)) {
    return NULL;
  }
  JS_ClearPendingException(cx);

  std::stringstream stackStream;
  JS::RootedObject stackObj(cx, exceptionStack.stack());
  if (stackObj.get()) {
    JS::RootedString stackStr(cx);
    JS::BuildStackString(cx, nullptr, stackObj, &stackStr, 2, js::StackFormat::SpiderMonkey);
    JS::RootedValue stackStrVal(cx, JS::StringValue(stackStr));
    stackStream << "\nJS Stack Trace:\n" << StrType::getValue(cx, stackStrVal);
  }


  // Gather Python context
  PyObject *pyErrType = PyObject_Type(exceptionValue);
  const char *pyErrTypeName = _PyType_Name((PyTypeObject *)pyErrType);

  PyObject *pyErrMsg = PyObject_Str(exceptionValue);

  if (traceBack) {
    _PyUnicodeWriter writer;
    _PyUnicodeWriter_Init(&writer);

    PyObject *fileName = NULL;
    int lineno = -1;

    PyTracebackObject *tb = (PyTracebackObject *)traceBack;

    long limit = PyTraceBack_LIMIT;

    PyObject *limitv = PySys_GetObject("tracebacklimit");
    if (limitv && PyLong_Check(limitv)) {
      int overflow;
      limit = PyLong_AsLongAndOverflow(limitv, &overflow);
      if (overflow > 0) {
        limit = LONG_MAX;
      }
      else if (limit <= 0) {
        return NULL;
      }
    }

    PyCodeObject *code = NULL;
    Py_ssize_t depth = 0;
    PyObject *last_file = NULL;
    int last_line = -1;
    PyObject *last_name = NULL;
    long cnt = 0;
    PyTracebackObject *tb1 = tb;
    int err = 0;

    int res;
    PyObject *line = PyUnicode_FromString("Traceback (most recent call last):\n");
    if (line == NULL) {
      goto error;
    }
    res = _PyUnicodeWriter_WriteStr(&writer, line);
    Py_DECREF(line);
    if (res < 0) {
      goto error;
    }

    // TODO should we reverse the stack and put it in the more common, non-python, top-most to bottom-most order? Wait for user feedback on experience
    while (tb1 != NULL) {
      depth++;
      tb1 = tb1->tb_next;
    }
    while (tb != NULL && depth > limit) {
      depth--;
      tb = tb->tb_next;
    }

#if PY_VERSION_HEX >= 0x03090000

    while (tb != NULL) {
      code = PyFrame_GetCode(tb->tb_frame);

      int tb_lineno = tb->tb_lineno;
      if (tb_lineno == -1) {
        tb_lineno = tb_get_lineno(tb);
      }

      if (last_file == NULL ||
          code->co_filename != last_file ||
          last_line == -1 || tb_lineno != last_line ||
          last_name == NULL || code->co_name != last_name) {

        if (cnt > TB_RECURSIVE_CUTOFF) {
          if (tb_print_line_repeated(&writer, cnt) < 0) {
            goto error;
          }
        }
        last_file = code->co_filename;
        last_line = tb_lineno;
        last_name = code->co_name;
        cnt = 0;
      }

      cnt++;

      if (cnt <= TB_RECURSIVE_CUTOFF) {
        fileName = code->co_filename;
        lineno = tb_lineno;

        line = PyUnicode_FromFormat("File \"%U\", line %d, in %U\n", fileName, lineno, code->co_name);
        if (line == NULL) {
          goto error;
        }

        int res = _PyUnicodeWriter_WriteStr(&writer, line);
        Py_DECREF(line);
        if (res < 0) {
          goto error;
        }
      }

      Py_CLEAR(code);
      tb = tb->tb_next;
    }
    if (cnt > TB_RECURSIVE_CUTOFF) {
      if (tb_print_line_repeated(&writer, cnt) < 0) {
        goto error;
      }
    }

#else

    while (tb != NULL && err == 0) {
      if (last_file == NULL ||
          tb->tb_frame->f_code->co_filename != last_file ||
          last_line == -1 || tb->tb_lineno != last_line ||
          last_name == NULL || tb->tb_frame->f_code->co_name != last_name) {
        if (cnt > TB_RECURSIVE_CUTOFF) {
          err = tb_print_line_repeated(&writer, cnt);
        }
        last_file = tb->tb_frame->f_code->co_filename;
        last_line = tb->tb_lineno;
        last_name = tb->tb_frame->f_code->co_name;
        cnt = 0;
      }
      cnt++;
      if (err == 0 && cnt <= TB_RECURSIVE_CUTOFF) {
        fileName = tb->tb_frame->f_code->co_filename;
        lineno = tb->tb_lineno;

        line = PyUnicode_FromFormat("File \"%U\", line %d, in %U\n", fileName, lineno, tb->tb_frame->f_code->co_name);
        if (line == NULL) {
          goto error;
        }

        int res = _PyUnicodeWriter_WriteStr(&writer, line);
        Py_DECREF(line);
        if (res < 0) {
          goto error;
        }
      }
      tb = tb->tb_next;
    }
    if (err == 0 && cnt > TB_RECURSIVE_CUTOFF) {
      err = tb_print_line_repeated(&writer, cnt);
    }

    if (err) {
      goto error;
    }

#endif

    {
      std::stringstream msgStream;
      msgStream << "Python " << pyErrTypeName << ": " << PyUnicode_AsUTF8(pyErrMsg) << "\n" << PyUnicode_AsUTF8(_PyUnicodeWriter_Finish(&writer));
      msgStream << stackStream.str();

      JS::RootedValue rval(cx);
      JS::RootedString filename(cx, JS_NewStringCopyZ(cx, PyUnicode_AsUTF8(fileName)));
      JS::RootedString message(cx, JS_NewStringCopyZ(cx, msgStream.str().c_str()));
      // stack argument cannot be passed in as a string anymore (deprecated), and could not find a proper example using the new argument type
      if (!JS::CreateError(cx, JSExnType::JSEXN_ERR, nullptr, filename, lineno, JS::ColumnNumberOneOrigin(1), nullptr, message, JS::NothingHandleValue, &rval)) {
        return NULL;
      }

      Py_DECREF(pyErrType);
      Py_DECREF(pyErrMsg);

      return rval.toObjectOrNull();
    }

  error:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_XDECREF(code);
  }

  // gather additional JS context details
  JS::ErrorReportBuilder reportBuilder(cx);
  if (!reportBuilder.init(cx, exceptionStack, JS::ErrorReportBuilder::WithSideEffects)) {
    return NULL;
  }
  JSErrorReport *errorReport = reportBuilder.report();

  std::stringstream msgStream;
  msgStream << "Python " << pyErrTypeName << ": " << PyUnicode_AsUTF8(pyErrMsg);
  msgStream << stackStream.str();

  JS::RootedValue rval(cx);
  JS::RootedString filename(cx, JS_NewStringCopyZ(cx, "")); // cannot be null or omitted, but is overriden by the errorReport
  JS::RootedString message(cx, JS_NewStringCopyZ(cx, msgStream.str().c_str()));
  // filename cannot be null
  if (!JS::CreateError(cx, JSExnType::JSEXN_ERR, nullptr, filename, 0, JS::ColumnNumberOneOrigin(1), errorReport, message, JS::NothingHandleValue, &rval)) {
    return NULL;
  }

  Py_DECREF(pyErrType);
  Py_DECREF(pyErrMsg);

  return rval.toObjectOrNull();
}