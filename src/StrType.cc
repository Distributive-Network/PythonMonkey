#include "include/StrType.hh"

#include "include/PyType.hh"

#include <Python.h>

#include <jsapi.h>
#include <js/String.h>

#define PY_UNICODE_HAS_WSTR (PY_VERSION_HEX < 0x030c0000) // Python version is less than 3.12

#define HIGH_SURROGATE_START 0xD800
#define HIGH_SURROGATE_END 0xDBFF
#define LOW_SURROGATE_START 0xDC00
#define LOW_SURROGATE_END 0xDFFF

#define PY_ASCII_OBJECT_CAST(op) ((PyASCIIObject *)(op))
#define PY_COMPACT_UNICODE_OBJECT_CAST(op) ((PyCompactUnicodeObject *)(op))
#define PY_UNICODE_OBJECT_CAST(op) ((PyUnicodeObject *)(op))

// https://github.com/python/cpython/blob/8de607a/Objects/unicodeobject.c#L114-L154
#define PY_UNICODE_OBJECT_UTF8(op)        (PY_COMPACT_UNICODE_OBJECT_CAST(op)->utf8)
#define PY_UNICODE_OBJECT_UTF8_LENGTH(op) (PY_COMPACT_UNICODE_OBJECT_CAST(op)->utf8_length)
#define PY_UNICODE_OBJECT_DATA_ANY(op)    (PY_UNICODE_OBJECT_CAST(op)->data.any)
#define PY_UNICODE_OBJECT_DATA_UCS2(op)   (PY_UNICODE_OBJECT_CAST(op)->data.ucs2)
#define PY_UNICODE_OBJECT_HASH(op)        (PY_ASCII_OBJECT_CAST(op)->hash)
#define PY_UNICODE_OBJECT_STATE(op)       (PY_ASCII_OBJECT_CAST(op)->state)
#define PY_UNICODE_OBJECT_KIND(op)        (PY_ASCII_OBJECT_CAST(op)->state.kind)
#define PY_UNICODE_OBJECT_LENGTH(op)      (PY_ASCII_OBJECT_CAST(op)->length)
#if PY_UNICODE_HAS_WSTR
  #define PY_UNICODE_OBJECT_WSTR(op)        (PY_ASCII_OBJECT_CAST(op)->wstr)
  #define PY_UNICODE_OBJECT_WSTR_LENGTH(op) (PY_COMPACT_UNICODE_OBJECT_CAST(op)->wstr_length)
  #define PY_UNICODE_OBJECT_READY(op)       (PY_ASCII_OBJECT_CAST(op)->state.ready)
#endif

StrType::StrType(PyObject *object) : PyType(object) {}

StrType::StrType(char *string) : PyType(Py_BuildValue("s", string)) {}

StrType::StrType(JSContext *cx, JSString *str) {
  JSLinearString *lstr = JS_EnsureLinearString(cx, str);
  JS::AutoCheckCannotGC nogc;
  PyObject *p;

  size_t length = JS::GetLinearStringLength(lstr);

  pyObject = (PyObject *)PyObject_New(PyUnicodeObject, &PyUnicode_Type); // new reference
  Py_INCREF(pyObject); // XXX: Why?

  // Initialize as legacy string (https://github.com/python/cpython/blob/v3.12.0b1/Include/cpython/unicodeobject.h#L78-L93)
  // see https://github.com/python/cpython/blob/v3.11.3/Objects/unicodeobject.c#L1230-L1245
  PY_UNICODE_OBJECT_HASH(pyObject) = -1;
  PY_UNICODE_OBJECT_STATE(pyObject).interned = 0;
  PY_UNICODE_OBJECT_STATE(pyObject).compact = 0;
  PY_UNICODE_OBJECT_STATE(pyObject).ascii = 0;
  PY_UNICODE_OBJECT_UTF8(pyObject) = NULL;
  PY_UNICODE_OBJECT_UTF8_LENGTH(pyObject) = 0;

  if (JS::LinearStringHasLatin1Chars(lstr)) { // latin1 spidermonkey, latin1 python
    const JS::Latin1Char *chars = JS::GetLatin1LinearStringChars(nogc, lstr);

    PY_UNICODE_OBJECT_DATA_ANY(pyObject) = (void *)chars;
    PY_UNICODE_OBJECT_KIND(pyObject) = PyUnicode_1BYTE_KIND;
    PY_UNICODE_OBJECT_LENGTH(pyObject) = length;
  #if PY_UNICODE_HAS_WSTR
    PY_UNICODE_OBJECT_WSTR(pyObject) = NULL;
    PY_UNICODE_OBJECT_WSTR_LENGTH(pyObject) = 0;
    PY_UNICODE_OBJECT_READY(pyObject) = 1;
  #endif
  }
  else { // utf16 spidermonkey, ucs2 python
    const char16_t *chars = JS::GetTwoByteLinearStringChars(nogc, lstr);

    PY_UNICODE_OBJECT_DATA_ANY(pyObject) = (void *)chars;
    PY_UNICODE_OBJECT_KIND(pyObject) = PyUnicode_2BYTE_KIND;
    PY_UNICODE_OBJECT_LENGTH(pyObject) = length;

  #if PY_UNICODE_HAS_WSTR
    // python unicode objects take advantage of a possible performance gain on systems where
    // sizeof(wchar_t) == 2, i.e. Windows systems if the string is using UCS2 encoding by setting the
    // wstr pointer to point to the same data as the data.any pointer.
    // On systems where sizeof(wchar_t) == 4, i.e. Unixy systems, a similar performance gain happens if the
    // string is using UCS4 encoding [this is automatically handled by asUCS4()]
    if (sizeof(wchar_t) == 2) {
      PY_UNICODE_OBJECT_WSTR(pyObject) = (wchar_t *)chars;
      PY_UNICODE_OBJECT_WSTR_LENGTH(pyObject) = length;
    }
    else {
      PY_UNICODE_OBJECT_WSTR(pyObject) = NULL;
      PY_UNICODE_OBJECT_WSTR_LENGTH(pyObject) = 0;
    }
    PY_UNICODE_OBJECT_READY(pyObject) = 1;
  #endif
  }
}

const char *StrType::getValue() const {
  return PyUnicode_AsUTF8(pyObject);
}

PyObject *StrType::asUCS4() {
  uint16_t *chars = PY_UNICODE_OBJECT_DATA_UCS2(pyObject);
  ssize_t length = PY_UNICODE_OBJECT_LENGTH(pyObject);

  uint32_t ucs4String[length];
  size_t ucs4Length = 0;

  for (ssize_t i = 0; i < length; i++) {
    if (chars[i] >= LOW_SURROGATE_START && chars[i] <= LOW_SURROGATE_END) // character is an unpaired low surrogate
    {
      char hexString[5];
      sprintf(hexString, "%x", (unsigned int)chars[i]);
      std::string errorString = std::string("string contains an unpaired low surrogate at position: ") + std::to_string(i) + std::string(" with a value of 0x") + hexString;
      PyErr_SetString(PyExc_UnicodeTranslateError, errorString.c_str());
      return NULL;
    }
    else if (chars[i] >= HIGH_SURROGATE_START && chars[i] <= HIGH_SURROGATE_END) { // character is a high surrogate
      if ((i + 1 < length) && chars[i+1] >= LOW_SURROGATE_START && chars[i+1] <= LOW_SURROGATE_END) { // next character is a low surrogate
        // see https://www.unicode.org/faq/utf_bom.html#utf16-3 for details
        uint32_t X = (chars[i] & ((1 << 6) -1)) << 10 | chars[i+1] & ((1 << 10) -1);
        uint32_t W = (chars[i] >> 6) & ((1 << 5) - 1);
        uint32_t U = W+1;
        ucs4String[ucs4Length] = U << 16 | X;
        ucs4Length++;
        i++; // skip over low surrogate
      }
      else { // next character is not a low surrogate
        char hexString[5];
        sprintf(hexString, "%x", (unsigned int)chars[i]);
        std::string errorString = std::string("string contains an unpaired high surrogate at position: ") + std::to_string(i) + std::string(" with a value of 0x") + hexString;
        PyErr_SetString(PyExc_UnicodeTranslateError, errorString.c_str());
        return NULL;
      }
    }
    else { // character is not a surrogate, and is in the BMP
      ucs4String[ucs4Length] = chars[i];
      ucs4Length++;
    }
  }

  return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, ucs4String, ucs4Length);
}