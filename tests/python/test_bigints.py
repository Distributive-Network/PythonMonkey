import pytest
import pythonmonkey as pm
import random


def test_eval_numbers_bigints():
  def test_bigint(py_number: int):
    js_number = pm.eval(f'{repr(py_number)}n')
    assert py_number == js_number

  test_bigint(0)
  test_bigint(1)
  test_bigint(-1)

  # CPython would reuse the objects for small ints in range [-5, 256]
  # Making sure we don't do any changes on them
  def test_cached_int_object(py_number):

    # type is still int
    assert type(py_number) is int
    assert not isinstance(py_number, pm.bigint)
    test_bigint(py_number)
    assert type(py_number) is int
    assert not isinstance(py_number, pm.bigint)
    # the value doesn't change
   # TODO (Tom Tang): Find a way to create a NEW int object with the same
   # value, because int literals also reuse the cached int objects
  for _ in range(2):
    test_cached_int_object(0)  # _PyLong_FromByteArray reuses the int 0 object,
    # see https://github.com/python/cpython/blob/3.9/Objects/longobject.c#L862
    for i in range(10):
      test_cached_int_object(random.randint(-5, 256))

  test_bigint(18014398509481984)  # 2**54
  test_bigint(-18014398509481984)     # -2**54
  test_bigint(18446744073709551615)  # 2**64-1
  test_bigint(18446744073709551616)  # 2**64
  test_bigint(-18446744073709551617)  # -2**64-1

  limit = 2037035976334486086268445688409378161051468393665936250636140449354381299763336706183397376
  #     = 2**300
  for i in range(10):
    py_number = random.randint(-limit, limit)
    test_bigint(py_number)

  # TODO (Tom Tang): test -0 (negative zero)
  # There's no -0 in both Python int and JS BigInt,
  # but this could be possible in JS BigInt's internal representation as it uses a sign bit flag.
  # On the other hand, Python int uses `ob_size` 0 for 0, >0 for positive values, <0 for negative values


def test_eval_boxed_numbers_bigints():
  def test_boxed_bigint(py_number: int):
    # `BigInt()` can only be called without `new`
    #   https://tc39.es/ecma262/#sec-bigint-constructor
    js_number = pm.eval(f'new Object({repr(py_number)}n)')
    assert py_number == js_number

  test_boxed_bigint(0)
  test_boxed_bigint(1)
  test_boxed_bigint(-1)

  limit = 2037035976334486086268445688409378161051468393665936250636140449354381299763336706183397376
  #     = 2**300
  for i in range(10):
    py_number = random.randint(-limit, limit)
    test_boxed_bigint(py_number)


def test_eval_functions_bigints():
  ident = pm.eval("(a) => { return a }")
  add = pm.eval("(a, b) => { return a + b }")

  int1 = random.randint(-1000000, 1000000)
  bigint1 = pm.bigint(int1)
  assert int1 == bigint1

  # should return pm.bigint
  assert type(ident(bigint1)) == pm.bigint
  assert ident(bigint1) is not bigint1
  # should return float (because JS number is float64)
  assert type(ident(int1)) == float
  assert ident(int1) == ident(bigint1)

  # should raise exception on ints > (2^53-1), or < -(2^53-1)
  def not_raise(num):
    ident(num)

  def should_raise(num):
    with pytest.raises(OverflowError, match="Use pythonmonkey.bigint instead"):
      ident(num)
  # autopep8: off
  not_raise(9007199254740991)      # +(2**53-1),     0x433_FFFFFFFFFFFFF in float64
  should_raise(9007199254740992)   # +(2**53  ),     0x434_0000000000000 in float64
  should_raise(9007199254740993)   # +(2**53+1), NOT 0x434_0000000000001 (2**53+2)
  not_raise(-9007199254740991)     # -(2**53-1)
  should_raise(-9007199254740992)  # -(2**53  )
  should_raise(-9007199254740993)  # -(2**53+1)
  # autopep8: on

  # should also raise exception on large integers (>=2**53) that can be exactly represented by a float64
  #   in our current implementation
  # autopep8: off
  should_raise(9007199254740994)  # 2**53+2, 0x434_0000000000001 in float64
  should_raise(2**61 + 2**9)      #          0x43C_0000000000001 in float64
  # autopep8: on

  # should raise "Use pythonmonkey.bigint" instead of `PyLong_AsLongLong`'s
  # "OverflowError: int too big to convert" on ints larger than 64bits
  should_raise(2**65)
  should_raise(-2**65)
  not_raise(pm.bigint(2**65))
  not_raise(pm.bigint(-2**65))

  # should raise JS error when mixing a BigInt with a number in arithmetic operations
  def should_js_error(a, b):
    with pytest.raises(pm.SpiderMonkeyError, match="can't convert BigInt to number"):
      add(a, b)
  should_js_error(pm.bigint(0), 0)
  should_js_error(pm.bigint(1), 2)
  should_js_error(3, pm.bigint(4))
  should_js_error(-5, pm.bigint(6))

  assert add(pm.bigint(0), pm.bigint(0)) == 0
  assert add(pm.bigint(1), pm.bigint(0)) == 1
  assert add(pm.bigint(1), pm.bigint(2)) == 3
  assert add(pm.bigint(-1), pm.bigint(1)) == 0
  assert add(pm.bigint(2**60), pm.bigint(0)) == 1152921504606846976
  assert add(pm.bigint(2**65), pm.bigint(-2**65 - 1)) == -1

  # fuzztest
  limit = 2037035976334486086268445688409378161051468393665936250636140449354381299763336706183397376  # 2**300
  for i in range(10):
    num1 = random.randint(-limit, limit)
    num2 = random.randint(-limit, limit)
    assert add(pm.bigint(num1), pm.bigint(num2)) == num1 + num2


def test_eval_functions_bigint_factorial():
  factorial = pm.eval("(num) => {let r = 1n; for(let i = 0n; i<num; i++){r *= num - i}; return r}")
  assert factorial(pm.bigint(1)) == 1
  assert factorial(pm.bigint(18)) == 6402373705728000
  assert factorial(pm.bigint(19)) == 121645100408832000  # > Number.MAX_SAFE_INTEGER
  assert factorial(pm.bigint(21)) == 51090942171709440000  # > 64 bit int
  assert factorial(pm.bigint(35)) == 10333147966386144929666651337523200000000  # > 128 bit


def test_eval_functions_bigint_crc32():
  crc_table_at = pm.eval("""
    // translated from https://rosettacode.org/wiki/CRC-32#Python
    const crc_table = (function create_table() {
        const a = []
        for (let i = 0n; i < 256n; i++) {
            let k = i
            for (let j = 0n; j < 8n; j++) {
                // must use bigint here as js number is trimmed to int32 in bitwise operations
                if (k & 1n) k ^= 0x1db710640n
                k >>= 1n
            }
            a.push(k)
        }
        return a
    })();
    (n) => crc_table[n]
    """)
  assert type(crc_table_at(1)) == pm.bigint
  assert crc_table_at(0) == 0
  assert crc_table_at(1) == 1996959894
  assert crc_table_at(255) == 755167117  # last item
