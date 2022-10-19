#include "include/StrType.hh"

#include "include/PyType.hh"

#include <Python.h>

#include <jsapi.h>
#include <js/String.h>

#include <iostream>

#define HIGH_SURROGATE_START 0xD800
#define HIGH_SURROGATE_END 0xDBFF
#define LOW_SURROGATE_START 0xDC00
#define LOW_SURROGATE_END 0xDFFF

StrType::StrType(PyObject *object) : PyType(object) {}

StrType::StrType(char *string) : PyType(Py_BuildValue("s", string)) {}

StrType::StrType(JSContext *cx, JSString *str) {
  JSLinearString *lstr = JS_EnsureLinearString(cx, str);
  JS::AutoCheckCannotGC nogc;
  PyObject *p;

  size_t length = JS::GetLinearStringLength(lstr);
  pyObject = PyUnicode_FromStringAndSize(NULL, length);

  Py_XINCREF(pyObject);

  // need to free memory malloc'd by PyUnicodeObject, otherwise program will crash when python quits
  free((*(PyUnicodeObject *)pyObject).data.any);

  if (JS::LinearStringHasLatin1Chars(lstr)) { // latin1 spidermonkey, latin1 python
    const JS::Latin1Char *chars = JS::GetLatin1LinearStringChars(nogc, lstr);

    // TODO (Caleb Aikens) check if there are any python-made macros that make this less ugly
    (*(PyUnicodeObject *)pyObject).data.any = (void *)chars;
    (*(PyUnicodeObject *)pyObject)._base._base.state.kind = PyUnicode_1BYTE_KIND;
    (*(PyUnicodeObject *)pyObject)._base._base.length = length;
    (*(PyUnicodeObject *)pyObject)._base._base.wstr = NULL;
    (*(PyUnicodeObject *)pyObject)._base.wstr_length = 0;
    (*(PyUnicodeObject *)pyObject)._base._base.state.ready = 1;
  }
  else { // utf16 spidermonkey, ucs2 python
    const char16_t *chars = JS::GetTwoByteLinearStringChars(nogc, lstr);

    // TODO (Caleb Aikens) check if there are any python-made macros that make this less ugly
    (*(PyUnicodeObject *)pyObject).data.any = (void *)chars;
    (*(PyUnicodeObject *)pyObject)._base._base.state.kind = PyUnicode_2BYTE_KIND;
    (*(PyUnicodeObject *)pyObject)._base._base.length = length;

    if (sizeof(wchar_t) == 2) {
      (*(PyUnicodeObject *)pyObject)._base._base.wstr = (wchar_t *)chars;
      (*(PyUnicodeObject *)pyObject)._base.wstr_length = length;
    }
    else {
      (*(PyUnicodeObject *)pyObject)._base._base.wstr = NULL;
      (*(PyUnicodeObject *)pyObject)._base.wstr_length = 0;
    }

    (*(PyUnicodeObject *)pyObject)._base._base.state.ready = 1;
  }
}

const char *StrType::getValue() const {
  return PyUnicode_AsUTF8(pyObject);
}

bool StrType::containsSurrogatePair() {
  if ((*(PyUnicodeObject *)pyObject)._base._base.state.kind != PyUnicode_2BYTE_KIND) { // if the string is not UCS2-encoded
    return false;
  }

  // TODO (Caleb Aikens) check if there are any python-made macros that make this less ugly
  Py_UCS2 *chars = (*(PyUnicodeObject *)pyObject).data.ucs2;
  ssize_t length = (*(PyUnicodeObject *)pyObject)._base._base.length;

  for (size_t i = 0; i < length; i++) {
    if (chars[i] >= HIGH_SURROGATE_START && chars[i] <= HIGH_SURROGATE_END && chars[i+1] >= LOW_SURROGATE_START && chars[i+1] <= LOW_SURROGATE_END) {
      return true;
    }
    return false;
  }
}

void StrType::asUCS4() {
  if (!this->containsSurrogatePair()) {
    return;
  }

   // TODO (Caleb Aikens) check if there are any python-made macros that make this less ugly
  uint16_t *chars = (*(PyUnicodeObject *)pyObject).data.ucs2;
  ssize_t length = (*(PyUnicodeObject *)pyObject)._base._base.length;

  uint32_t ucs4String[length];
  size_t ucs4Length = 0;

  for (ssize_t i = 0; i < length; i++) {
    if (chars[i] >= LOW_SURROGATE_START && chars[i] <= LOW_SURROGATE_END) // character is an unpaired low surrogate
    {
      //TODO (Caleb Aikens) raise an exception here
    }
    else if (chars[i] >= HIGH_SURROGATE_START && chars[i] <= HIGH_SURROGATE_END) { // character is a high surrogate
      if ((i + 1 < length) && chars[i+1] >= LOW_SURROGATE_START && chars[i+1] <= LOW_SURROGATE_END) { // next character is a low surrogate
        //see https://www.unicode.org/faq/utf_bom.html#utf16-3 for details
        uint32_t X = (chars[i] & ((1 << 6) -1)) << 10 | chars[i+1] & ((1 << 10) -1);
        uint32_t W = (chars[i] >> 6) & ((1 << 5) - 1);
        uint32_t U = W+1;
        ucs4String[ucs4Length] = U << 16 | X;
        ucs4Length++;
        i++; // skip over low surrogate
      }
      else { // next character is not a low surrogate
        // TODO (Caleb Aikens) raise an exception here
      }
    }
    else { //character is not a surrogate, and is in the BMP
      ucs4String[ucs4Length] = chars[i];
      ucs4Length++;
    }
  }

  Py_XDECREF(pyObject);
  pyObject = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, ucs4String, ucs4Length);
  Py_XINCREF(pyObject);
}

void StrType::print(std::ostream &os) const {
  os << "'" << this->getValue() << "'";
}