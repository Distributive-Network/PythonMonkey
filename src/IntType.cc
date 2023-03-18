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
#define JS_DIGIT_BYTE (sizeof(uintptr_t)/sizeof(uint8_t))

IntType::IntType(PyObject *object) : PyType(object) {}

IntType::IntType(long n) : PyType(Py_BuildValue("i", n)) {}

IntType::IntType(JSContext *cx, JS::BigInt *bigint) {
  // Get the sign bit
  bool isNegative = BigIntIsNegative(bigint);

  // Read the digits count in the JS BigInt
  // https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/BigIntType.h#l48
  // https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/gc/Cell.h#l623
  uint32_t jsDigitCount = ((uint32_t *)bigint)[1];

  // Get all the 64-bit (assuming we compile on 64-bit OS) "digits" from JS BigInt
  uintptr_t *jsDigits = (uintptr_t *)(((char *)bigint) + CELL_HEADER_LENGTH);
  //
  // The digit storage starts with the least significant digit (little-endian digit order).
  // Byte order within a digit is native-endian.

  if constexpr (std::endian::native == std::endian::big) { // C++20
    // TODO: use C++23 std::byteswap?
    printf("big-endian cpu is not supported by PythonMonkey yet");
    return;
  }
  // If the native endianness is also little-endian,
  // we now have uniform bytes of 8-bit "digits" in little-endian order
  auto bytes = const_cast<const uint8_t *>((uint8_t *)jsDigits);
  pyObject = _PyLong_FromByteArray(bytes, jsDigitCount * JS_DIGIT_BYTE, true, false);
  // FIXME: sign
}

void IntType::print(std::ostream &os) const {
  // Making sure the value does not overflow even if the int has millions of bits of precision
  // FIXME double still overflows at 1.7976931348623157E+308
  // TODO (Tom Tang) use Python's `str` conversion and then use `PyUnicode_AsUTF8` to print with whole precisions
  os << PyLong_AsDouble(pyObject);
}