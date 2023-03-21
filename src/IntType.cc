#include "include/modules/pythonmonkey/pythonmonkey.hh"

#include "include/IntType.hh"

#include "include/PyType.hh"
#include "include/TypeEnum.hh"

#include <jsapi.h>
#include <js/BigInt.h>

#include <Python.h>

#include <iostream>
#include <bit>

#define CELL_HEADER_LENGTH 8 // https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/gc/Cell.h#l602

#define JS_DIGIT_BIT JS_BITS_PER_WORD
#define PY_DIGIT_BIT PYLONG_BITS_IN_DIGIT

#define js_digit_t uintptr_t // https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/BigIntType.h#l36
#define JS_DIGIT_BYTE (sizeof(js_digit_t)/sizeof(uint8_t))

#define JS_INLINE_DIGIT_MAX_LEN 1 // https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/BigIntType.h#l43

IntType::IntType(PyObject *object) : PyType(object) {}

IntType::IntType(long n) : PyType(Py_BuildValue("i", n)) {}

IntType::IntType(JSContext *cx, JS::BigInt *bigint) {
  // Get the sign bit
  bool isNegative = BigIntIsNegative(bigint);

  // Read the digits count in this JS BigInt
  //    see https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/BigIntType.h#l48
  //        https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/gc/Cell.h#l623
  uint32_t jsDigitCount = ((uint32_t *)bigint)[1];

  // Get all the 64-bit (assuming we compile on 64-bit OS) "digits" from JS BigInt
  js_digit_t *jsDigits = (js_digit_t *)(((char *)bigint) + CELL_HEADER_LENGTH);
  if (jsDigitCount > JS_INLINE_DIGIT_MAX_LEN) { // hasHeapDigits
    // We actually have a pointer to the digit storage if the number cannot fit in one uint64_t
    //    see https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/BigIntType.h#l54
    jsDigits = *((js_digit_t **)jsDigits);
  }
  //
  // The digit storage starts with the least significant digit (little-endian digit order).
  // Byte order within a digit is native-endian.

  if constexpr (std::endian::native == std::endian::big) { // C++20
    // TODO: use C++23 std::byteswap?
    printf("big-endian cpu is not supported by PythonMonkey yet");
    return;
  }
  // If the native endianness is also little-endian,
  // we now have consecutive bytes of 8-bit "digits" in little-endian order
  auto bytes = const_cast<const uint8_t *>((uint8_t *)jsDigits);
  pyObject = _PyLong_FromByteArray(bytes, jsDigitCount * JS_DIGIT_BYTE, true, false);

  // Set the sign bit
  //    see https://github.com/python/cpython/blob/3.9/Objects/longobject.c#L956
  if (isNegative) {
    auto pyDigitCount = Py_SIZE(pyObject);
    Py_SET_SIZE(pyObject, -pyDigitCount);
  }

  // Cast to a pythonmonkey.bigint to differentiate it from a normal Python int,
  //  allowing Py<->JS two-way BigInt conversion
  Py_SET_TYPE(pyObject, (PyTypeObject *)(PythonMonkey_BigInt));
}

void IntType::print(std::ostream &os) const {
  // Making sure the value does not overflow even if the int has millions of bits of precision
  auto str = PyObject_Str(pyObject);
  os << PyUnicode_AsUTF8(str);
  // https://pythonextensionpatterns.readthedocs.io/en/latest/refcount.html#new-references
  Py_DECREF(str); // free
}