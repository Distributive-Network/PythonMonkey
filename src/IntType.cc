/**
 * @file IntType.cc
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network) & Tom Tang (xmader@distributive.network)
 * @brief Struct for representing python ints
 * @date 2023-03-16
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#include "include/modules/pythonmonkey/pythonmonkey.hh"
#include "include/IntType.hh"

#include <jsapi.h>
#include <js/BigInt.h>

#include <vector>

#define SIGN_BIT_MASK 0b1000 // https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/BigIntType.h#l40
#define CELL_HEADER_LENGTH 8 // https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/gc/Cell.h#l602

#define JS_DIGIT_BIT JS_BITS_PER_WORD
#define PY_DIGIT_BIT PYLONG_BITS_IN_DIGIT

#define js_digit_t uintptr_t // https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/BigIntType.h#l36
#define JS_DIGIT_BYTE (sizeof(js_digit_t)/sizeof(uint8_t))

#define JS_INLINE_DIGIT_MAX_LEN 1 // https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/BigIntType.h#l43

static const char HEX_CHAR_LOOKUP_TABLE[] = "0123456789ABCDEF";

/**
 * @brief Set the sign bit of a Python int
 * PyLongObject is no longer an extension of PyVarObject struct in Python 3.12+, so we have to handle them differently.
 * @see Compare https://github.com/python/cpython/blob/v3.12.0b1/Include/cpython/longintrepr.h#L82-L90 and https://github.com/python/cpython/blob/v3.11.3/Include/cpython/longintrepr.h#L82-L85
 * @param op - the Python int object
 * @param sign - -1 (negative), 0 (zero), or 1 (positive)
 */
static inline void PythonLong_SetSign(PyLongObject *op, int sign) {
#ifdef _PyLong_SIGN_MASK // Python 3.12+
  // see https://github.com/python/cpython/blob/v3.12.0b1/Include/internal/pycore_long.h#L214-L239
  op->long_value.lv_tag &= ~_PyLong_SIGN_MASK; // clear sign bits
  op->long_value.lv_tag |= (1-sign) & _PyLong_SIGN_MASK; // set the new sign bits value
#else // Python version is less than 3.12
  // see https://github.com/python/cpython/blob/v3.9.16/Objects/longobject.c#L956
  Py_ssize_t pyDigitCount = Py_SIZE(op);
  #if PY_VERSION_HEX >= 0x03090000
  Py_SET_SIZE(op, sign * std::abs(pyDigitCount));
  #else
  ((PyVarObject *)op)->ob_size = sign * std::abs(pyDigitCount); // Py_SET_SIZE is not available in Python < 3.9
  #endif
#endif
}

/**
 * @brief Test if the Python int is negative
 */
static inline bool PythonLong_IsNegative(const PyLongObject *op) {
#ifdef _PyLong_SIGN_MASK // Python 3.12+
  // see https://github.com/python/cpython/blob/v3.12.0b1/Include/internal/pycore_long.h#L163-L167
  #define _PyLong_SIGN_NEGATIVE 2; // Sign bits value = (1-sign), ie. negative=2, positive=0, zero=1.
                                   // https://github.com/python/cpython/blob/v3.12.0b1/Include/internal/pycore_long.h#L111-L118
  return (op->long_value.lv_tag & _PyLong_SIGN_MASK) == _PyLong_SIGN_NEGATIVE;
#else // Python version is less than 3.12
  // see https://github.com/python/cpython/blob/v3.9.16/Objects/longobject.c#L977
  Py_ssize_t pyDigitCount = Py_SIZE(op); // negative on negative numbers
  return pyDigitCount < 0;
#endif
}


PyObject *IntType::getPyObject(JSContext *cx, JS::BigInt *bigint) {
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

  #if not (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)) // gcc extensions (also supported by clang)
    #error "Big-endian cpu is not supported by PythonMonkey yet"
  // @TODO (Tom Tang): use C++23 std::byteswap?
  #endif

  // If the native endianness is also little-endian,
  // we now have consecutive bytes of 8-bit "digits" in little-endian order
  const uint8_t *bytes = const_cast<const uint8_t *>((uint8_t *)jsDigits);
  PyObject *pyIntObj = _PyLong_FromByteArray(bytes, jsDigitCount * JS_DIGIT_BYTE, true, false);

  // Cast to a pythonmonkey.bigint to differentiate it from a normal Python int,
  //  allowing Py<->JS two-way BigInt conversion.
  // We don't do `Py_SET_TYPE` because `_PyLong_FromByteArray` may cache and reuse objects for small ints
  #if PY_VERSION_HEX >= 0x03090000
  PyObject *pyObject = PyObject_CallOneArg(getPythonMonkeyBigInt(), pyIntObj); // pyObject = pythonmonkey.bigint(pyIntObj)
  #else
  PyObject *pyObject = PyObject_CallFunction(getPythonMonkeyBigInt(), "O", pyIntObj); // PyObject_CallOneArg is not available in Python < 3.9
  #endif
  Py_DECREF(pyIntObj);

  // Set the sign bit
  if (isNegative) {
    PythonLong_SetSign((PyLongObject *)pyObject, -1);
  }

  return pyObject;
}

JS::BigInt *IntType::toJsBigInt(JSContext *cx, PyObject *pyObject) {
  // Figure out how many 64-bit "digits" we would have for JS BigInt
  //    see https://github.com/python/cpython/blob/3.9/Modules/_randommodule.c#L306
  size_t bitCount = _PyLong_NumBits(pyObject);
  if (bitCount == (size_t)-1 && PyErr_Occurred())
    return nullptr;
  uint32_t jsDigitCount = bitCount == 0 ? 1 : (bitCount - 1) / JS_DIGIT_BIT + 1;
  // Get the sign bit
  bool isNegative = PythonLong_IsNegative((PyLongObject *)pyObject);
  // Force to make the number positive otherwise _PyLong_AsByteArray would complain
  if (isNegative) {
    PythonLong_SetSign((PyLongObject *)pyObject, 1);
  }

  JS::BigInt *bigint = nullptr;
  if (jsDigitCount <= 1) {
    // Fast path for int fits in one js_digit_t (uint64 on 64-bit OS)
    bigint = JS::detail::BigIntFromUint64(cx, PyLong_AsUnsignedLongLong(pyObject));
  } else {
    // Convert to bytes of 8-bit "digits" in **big-endian** order
    size_t byteCount = (size_t)JS_DIGIT_BYTE * jsDigitCount;
    uint8_t *bytes = (uint8_t *)PyMem_Malloc(byteCount);
    _PyLong_AsByteArray((PyLongObject *)pyObject, bytes, byteCount, /*is_little_endian*/ false, false);

    // Convert pm.bigint to JS::BigInt through hex strings (no public API to convert directly through bytes)
    // TODO (Tom Tang): We could manually allocate the memory, https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/BigIntType.cpp#l162, but still no public API
    // TODO (Tom Tang): Could we fill in an object with similar memory alignment (maybe by NewArrayBufferWithContents), and coerce it to BigInt?

    // Calculate the number of chars required to represent the bigint in hex string
    size_t charCount = byteCount * 2;
    // Convert bytes to hex string (big-endian)
    std::vector<char> chars = std::vector<char>(charCount); // can't be null-terminated, otherwise SimpleStringToBigInt would read the extra \0 character and then segfault
    for (size_t i = 0, j = 0; i < charCount; i += 2, j++) {
      chars[i] = HEX_CHAR_LOOKUP_TABLE[(bytes[j] >> 4)&0xf]; // high nibble
      chars[i+1] = HEX_CHAR_LOOKUP_TABLE[bytes[j]&0xf];      // low  nibble
    }
    PyMem_Free(bytes);

    // Convert hex string to JS::BigInt
    mozilla::Span<const char> strSpan = mozilla::Span<const char>(chars); // storing only a pointer to the underlying array and length
    bigint = JS::SimpleStringToBigInt(cx, strSpan, 16);
  }

  if (isNegative) {
    // Make negative number back negative
    PythonLong_SetSign((PyLongObject *)pyObject, -1);

    // Set the sign bit
    // https://hg.mozilla.org/releases/mozilla-esr102/file/tip/js/src/vm/BigIntType.cpp#l1801
    /* flagsField */ ((uint32_t *)bigint)[0] |= SIGN_BIT_MASK;
  }

  return bigint;
}