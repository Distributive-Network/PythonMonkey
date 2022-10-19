#include "include/StrType.hh"

#include "include/PyType.hh"

#include <Python.h>

#include <jsapi.h>
#include <js/String.h>

#include <iostream>

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
    if (chars[i] >= 0xD800 && chars[i] <= 0xDBFF && chars[i+1] >= 0xDC00 && chars[i+1] <= 0xDFFF) {
      return true;
    }
    return false;
  }
}

// TODO (Caleb Aikens) implement this
void asUCS4() {

}

void StrType::print(std::ostream &os) const {
  os << "'" << this->getValue() << "'";
}