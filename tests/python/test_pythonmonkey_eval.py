import pytest
import pythonmonkey as pm
import gc
import random
from datetime import datetime, timedelta
import math
import asyncio

def test_passes():
    assert True

def test_eval_ascii_string_matches_evaluated_string():
    py_ascii_string = "abc"
    js_ascii_string = pm.eval(repr(py_ascii_string))
    assert py_ascii_string == js_ascii_string

def test_eval_latin1_string_matches_evaluated_string():
    py_latin1_string = "a©Ð"
    js_latin1_string = pm.eval(repr(py_latin1_string))
    assert py_latin1_string == js_latin1_string

def test_eval_null_character_string_matches_evaluated_string():
    py_null_character_string = "a\x00©"
    js_null_character_string = pm.eval(repr(py_null_character_string))
    assert py_null_character_string == js_null_character_string

def test_eval_ucs2_string_matches_evaluated_string():
    py_ucs2_string = "ՄԸՋ"
    js_ucs2_string = pm.eval(repr(py_ucs2_string))
    assert py_ucs2_string == js_ucs2_string

def test_eval_unpaired_surrogate_string_matches_evaluated_string():
    py_unpaired_surrogate_string = "Ջ©\ud8fe"
    js_unpaired_surrogate_string = pm.eval(repr(py_unpaired_surrogate_string))
    assert py_unpaired_surrogate_string == js_unpaired_surrogate_string

def test_eval_ucs4_string_matches_evaluated_string():
    py_ucs4_string = "🀄🀛🜢"
    js_utf16_string = pm.eval(repr(py_ucs4_string))
    js_ucs4_string = pm.asUCS4(js_utf16_string)
    assert py_ucs4_string == js_ucs4_string

def test_eval_latin1_string_fuzztest():
    n = 10
    for _ in range(n):
        length = random.randint(0x0000, 0xFFFF)
        string1 = ''

        for _ in range(length):
            codepoint = random.randint(0x00, 0xFF)
            string1 += chr(codepoint) # add random chr in latin1 range
        
        
        INITIAL_STRING = string1
        m = 10
        for _ in range(m):
            string2 = pm.eval(repr(string1))
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            gc.collect()
            pm.collect()
            
            #garbage collection should not collect variables still in scope
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            string1 = string2
        assert INITIAL_STRING == string1 #strings should still match after a bunch of iterations through JS

def test_eval_ucs2_string_fuzztest():
    n = 10
    for _ in range(n):
        length = random.randint(0x0000, 0xFFFF)
        string1 = ''

        for _i in range(length):
            codepoint = random.randint(0x00, 0xFFFF)
            string1 += chr(codepoint) # add random chr in ucs2 range
        
        
        INITIAL_STRING = string1
        m = 10
        for _ in range(m):
            string2 = pm.eval(repr(string1))
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            gc.collect()
            pm.collect()
            
            #garbage collection should not collect variables still in scope
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            string1 = string2
        assert INITIAL_STRING == string1 #strings should still match after a bunch of iterations through JS

def test_eval_ucs4_string_fuzztest():
    n = 10
    for _ in range(n):
        length = random.randint(0x0000, 0xFFFF)
        string1 = ''

        for _ in range(length):
            codepoint = random.randint(0x010000, 0x10FFFF)
            string1 += chr(codepoint) # add random chr outside BMP
        
        
        INITIAL_STRING = string1
        m = 10
        for _ in range(m):
            utf16_string2 = pm.eval("'" + string1 + "'")
            string2 = pm.asUCS4(utf16_string2)
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            gc.collect()
            pm.collect()
            
            #garbage collection should not collect variables still in scope
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            string1 = string2
        assert INITIAL_STRING == string1 #strings should still match after a bunch of iterations through JS

def test_eval_numbers_floats():
    for _ in range(10):
        py_number = random.uniform(-1000000,1000000)
        js_number = pm.eval(repr(py_number))
        assert py_number == js_number

def test_eval_numbers_floats_nan():
    jsNaN = pm.eval("NaN")
    assert math.isnan(jsNaN)

def test_eval_numbers_floats_negative_zero():
    jsNegZero = pm.eval("-0")
    assert jsNegZero == 0
    assert jsNegZero == 0.0 # expected that -0.0 == 0.0 == 0
    # https://docs.python.org/3/library/math.html#math.copysign
    assert math.copysign(1.0, jsNegZero) == -1.0

def test_eval_numbers_floats_inf():
    jsPosInf = pm.eval("Infinity")
    jsNegInf = pm.eval("-Infinity")
    assert jsPosInf == float("+inf")
    assert jsNegInf == float("-inf")

def test_eval_numbers_integers():
    for _ in range(10):
        py_number = random.randint(-1000000,1000000)
        js_number = pm.eval(repr(py_number))
        assert py_number == js_number

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
        assert type(py_number) == int
        assert type(py_number) != pm.bigint
        test_bigint(py_number)
        assert type(py_number) == int
        assert type(py_number) != pm.bigint
        # the value doesn't change
        # TODO (Tom Tang): Find a way to create a NEW int object with the same value, because int literals also reuse the cached int objects
    for _ in range(2):
        test_cached_int_object(0) # _PyLong_FromByteArray reuses the int 0 object,
                                  # see https://github.com/python/cpython/blob/3.9/Objects/longobject.c#L862
        for i in range(10):
            test_cached_int_object(random.randint(-5, 256))

    test_bigint(18014398509481984)      #  2**54
    test_bigint(-18014398509481984)     # -2**54
    test_bigint(18446744073709551615)   #  2**64-1
    test_bigint(18446744073709551616)   #  2**64
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

def test_eval_booleans():
    py_bool = True
    js_bool = pm.eval('true')
    assert py_bool == js_bool
    py_bool = False
    js_bool = pm.eval('false')
    assert py_bool == js_bool

def test_eval_dates():
    MIN_YEAR = 1 # https://docs.python.org/3/library/datetime.html#datetime.MINYEAR
    MAX_YEAR = 2023
    start = datetime(MIN_YEAR, 1, 1, 00, 00, 00)
    years = MAX_YEAR - MIN_YEAR + 1
    end = start + timedelta(days=365 * years)
    for _ in range(10):
        py_date = start + (end - start) * random.random()
        # round to milliseconds precision because the smallest unit for js Date is 1ms
        py_date = py_date.replace(microsecond=round(py_date.microsecond, -3))
        js_date = pm.eval(f'new Date("{py_date.isoformat()}")')
        assert py_date == js_date

def test_eval_boxed_booleans():
    py_bool = True
    js_bool = pm.eval('new Boolean(true)')
    assert py_bool == js_bool
    py_bool = False
    js_bool = pm.eval('new Boolean(false)')
    assert py_bool == js_bool

def test_eval_boxed_numbers_floats():
    for _ in range(10):
        py_number = random.uniform(-1000000,1000000)
        js_number = pm.eval(f'new Number({repr(py_number)})')
        assert py_number == js_number

def test_eval_boxed_numbers_integers():
    for _ in range(10):
        py_number = random.randint(-1000000,1000000)
        js_number = pm.eval(f'new Number({repr(py_number)})')
        assert py_number == js_number

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

def test_eval_boxed_ascii_string_matches_evaluated_string():
    py_ascii_string = "abc"
    js_ascii_string = pm.eval(f'new String({repr(py_ascii_string)})')
    assert py_ascii_string == js_ascii_string

def test_eval_boxed_latin1_string_matches_evaluated_string():
    py_latin1_string = "a©Ð"
    js_latin1_string = pm.eval(f'new String({repr(py_latin1_string)})')
    assert py_latin1_string == js_latin1_string

def test_eval_boxed_null_character_string_matches_evaluated_string():
    py_null_character_string = "a\x00©"
    js_null_character_string = pm.eval(f'new String({repr(py_null_character_string)})')
    assert py_null_character_string == js_null_character_string

def test_eval_boxed_ucs2_string_matches_evaluated_string():
    py_ucs2_string = "ՄԸՋ"
    js_ucs2_string = pm.eval(f'new String({repr(py_ucs2_string)})')
    assert py_ucs2_string == js_ucs2_string

def test_eval_boxed_unpaired_surrogate_string_matches_evaluated_string():
    py_unpaired_surrogate_string = "Ջ©\ud8fe"
    js_unpaired_surrogate_string = pm.eval(f'new String({repr(py_unpaired_surrogate_string)})')
    assert py_unpaired_surrogate_string == js_unpaired_surrogate_string

def test_eval_boxed_ucs4_string_matches_evaluated_string():
    py_ucs4_string = "🀄🀛🜢"
    js_utf16_string = pm.eval(f'new String({repr(py_ucs4_string)})')
    js_ucs4_string = pm.asUCS4(js_utf16_string)
    assert py_ucs4_string == js_ucs4_string

def test_eval_boxed_latin1_string_fuzztest():
    n = 10
    for _ in range(n):
        length = random.randint(0x0000, 0xFFFF)
        string1 = ''

        for _ in range(length):
            codepoint = random.randint(0x00, 0xFF)
            string1 += chr(codepoint) # add random chr in latin1 range
        
        
        INITIAL_STRING = string1
        m = 10
        for _ in range(m):
            string2 = pm.eval(f'new String({repr(string1)})')
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            gc.collect()
            pm.collect()
            
            #garbage collection should not collect variables still in scope
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            string1 = string2
        assert INITIAL_STRING == string1 #strings should still match after a bunch of iterations through JS

def test_eval_boxed_ucs2_string_fuzztest():
    n = 10
    for _ in range(n):
        length = random.randint(0x0000, 0xFFFF)
        string1 = ''

        for _ in range(length):
            codepoint = random.randint(0x00, 0xFFFF)
            string1 += chr(codepoint) # add random chr in ucs2 range
        
        
        INITIAL_STRING = string1
        m = 10
        for _ in range(m):
            string2 = pm.eval(f'new String({repr(string1)})')
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            gc.collect()
            pm.collect()
            
            #garbage collection should not collect variables still in scope
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            string1 = string2
        assert INITIAL_STRING == string1 #strings should still match after a bunch of iterations through JS

def test_eval_boxed_ucs4_string_fuzztest():
    n = 10
    for _ in range(n):
        length = random.randint(0x0000, 0xFFFF)
        string1 = ''

        for _ in range(length):
            codepoint = random.randint(0x010000, 0x10FFFF)
            string1 += chr(codepoint) # add random chr outside BMP
        
        
        INITIAL_STRING = string1
        m = 10
        for _ in range(m):
            utf16_string2 = pm.eval(f'new String("{string1}")')
            string2 = pm.asUCS4(utf16_string2)
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            gc.collect()
            pm.collect()
            
            #garbage collection should not collect variables still in scope
            assert len(string1) == length
            assert len(string2) == length
            assert len(string1) == len(string2)
            assert string1 == string2

            string1 = string2
        assert INITIAL_STRING == string1 #strings should still match after a bunch of iterations through JS
        
def test_eval_exceptions():
    # should print out the correct error messages
    with pytest.raises(pm.SpiderMonkeyError, match='SyntaxError: "" literal not terminated before end of script'):
        pm.eval('"123')
    with pytest.raises(pm.SpiderMonkeyError, match="SyntaxError: missing } in compound statement"):
        pm.eval('{')
    with pytest.raises(pm.SpiderMonkeyError, match="TypeError: can't convert BigInt to number"):
        pm.eval('1n + 1')
    with pytest.raises(pm.SpiderMonkeyError, match="ReferenceError: RANDOM_VARIABLE is not defined"):
        pm.eval('RANDOM_VARIABLE')
    with pytest.raises(pm.SpiderMonkeyError, match="RangeError: invalid array length"):
        pm.eval('new Array(-1)')
    with pytest.raises(pm.SpiderMonkeyError, match="Error: abc"):
        # manually by the `throw` statement
        pm.eval('throw new Error("abc")')

    # ANYTHING can be thrown in JS
    with pytest.raises(pm.SpiderMonkeyError, match="uncaught exception: 9007199254740993"):
        pm.eval('throw 9007199254740993n') # 2**53+1
    with pytest.raises(pm.SpiderMonkeyError, match="uncaught exception: null"):
        pm.eval('throw null')
    with pytest.raises(pm.SpiderMonkeyError, match="uncaught exception: undefined"):
        pm.eval('throw undefined')
    with pytest.raises(pm.SpiderMonkeyError, match="uncaught exception: something from toString"):
        # (side effect) calls the `toString` method if an object is thrown
        pm.eval('throw { toString() { return "something from toString" } }')

    # convert JS Error object to a Python Exception object for later use (in a `raise` statement)
    js_err = pm.eval("new RangeError('to be raised in Python')")
    assert isinstance(js_err, BaseException)
    assert isinstance(js_err, Exception)
    assert type(js_err) == pm.SpiderMonkeyError
    with pytest.raises(pm.SpiderMonkeyError, match="RangeError: to be raised in Python"):
        raise js_err

    # convert Python Exception object to a JS Error object
    get_err_msg = pm.eval("(err) => err.message")
    assert "Python BufferError: ttt" == get_err_msg(BufferError("ttt"))
    js_rethrow = pm.eval("(err) => { throw err }")
    with pytest.raises(pm.SpiderMonkeyError, match="Error: Python BaseException: 123"):
        js_rethrow(BaseException("123"))

def test_eval_undefined():
    x = pm.eval("undefined")
    assert x == None

def test_eval_null():
    x = pm.eval("null")
    assert x == pm.null
    
def test_eval_functions():
    f = pm.eval("() => { return undefined }")
    assert f() == None

    g = pm.eval("() => { return null}")
    assert g() == pm.null

    h = pm.eval("(a, b) => {return a + b}")
    n = 10
    for _ in range(n):
        a = random.randint(-1000, 1000)
        b = random.randint(-1000, 1000)
        assert h(a, b) == (a + b)
    
    for _ in range (n):
        a = random.uniform(-1000.0, 1000.0)
        b = random.uniform(-1000.0, 1000.0)
        assert h(a, b) == (a + b)
    
    assert math.isnan(h(float("nan"), 1))
    assert math.isnan(h(float("+inf"), float("-inf")))

def test_eval_functions_latin1_string_args():
    concatenate = pm.eval("(a, b) => { return a + b}")
    n = 10
    for i in range(n):
        length1 = random.randint(0x0000, 0xFFFF)
        length2 = random.randint(0x0000, 0xFFFF)
        string1 = ''
        string2 = ''

        for j in range(length1):
            codepoint = random.randint(0x00, 0xFFFF)
            string1 += chr(codepoint) # add random chr in ucs2 range
        for j in range(length2):
            codepoint = random.randint(0x00, 0xFFFF)
            string2 += chr(codepoint)
        
        assert concatenate(string1, string2) == (string1 + string2)

def test_eval_functions_bigints():
    ident = pm.eval("(a) => { return a }")
    add = pm.eval("(a, b) => { return a + b }")

    int1 = random.randint(-1000000,1000000)
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
    not_raise(9007199254740991)     #   2**53-1, 0x433_FFFFFFFFFFFFF in float64
    should_raise(9007199254740992)  #   2**53,   0x434_0000000000000 in float64
    should_raise(9007199254740993)  #   2**53+1, NOT 0x434_0000000000001 (2**53+2)
    not_raise(-9007199254740991)    # -(2**53-1)
    should_raise(-9007199254740992) # -(2**53)
    should_raise(-9007199254740993) # -(2**53+1)

    # should also raise exception on large integers (>=2**53) that can be exactly represented by a float64
    #   in our current implementation
    should_raise(9007199254740994)  #   2**53+2, 0x434_0000000000001 in float64
    should_raise(2**61+2**9)        #            0x43C_0000000000001 in float64

    # should raise "Use pythonmonkey.bigint" instead of `PyLong_AsLongLong`'s "OverflowError: int too big to convert" on ints larger than 64bits
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
    assert add(pm.bigint(2**65), pm.bigint(-2**65-1)) == -1

    # fuzztest
    limit = 2037035976334486086268445688409378161051468393665936250636140449354381299763336706183397376 # 2**300
    for i in range(10):
        num1 = random.randint(-limit, limit)
        num2 = random.randint(-limit, limit)
        assert add(pm.bigint(num1), pm.bigint(num2)) == num1+num2

def test_eval_functions_bigint_factorial():
    factorial = pm.eval("(num) => {let r = 1n; for(let i = 0n; i<num; i++){r *= num - i}; return r}")
    assert factorial(pm.bigint(1)) == 1
    assert factorial(pm.bigint(18)) == 6402373705728000
    assert factorial(pm.bigint(19)) == 121645100408832000 # > Number.MAX_SAFE_INTEGER
    assert factorial(pm.bigint(21)) == 51090942171709440000 # > 64 bit int
    assert factorial(pm.bigint(35)) == 10333147966386144929666651337523200000000 # > 128 bit

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
    assert crc_table_at(255) == 755167117 # last item

def test_eval_functions_ucs2_string_args():
    concatenate = pm.eval("(a, b) => { return a + b}")
    n = 10
    for i in range(n):
        length1 = random.randint(0x0000, 0xFFFF)
        length2 = random.randint(0x0000, 0xFFFF)
        string1 = ''
        string2 = ''

        for j in range(length1):
            codepoint = random.randint(0x00, 0xFF)
            string1 += chr(codepoint) # add random chr in latin1 range
        for j in range(length2):
            codepoint = random.randint(0x00, 0xFF)
            string2 += chr(codepoint)
        
        assert concatenate(string1, string2) == (string1 + string2)

def test_eval_functions_ucs4_string_args():
    concatenate = pm.eval("(a, b) => { return a + b}")
    n = 10
    for i in range(n):
        length1 = random.randint(0x0000, 0xFFFF)
        length2 = random.randint(0x0000, 0xFFFF)
        string1 = ''
        string2 = ''

        for j in range(length1):
            codepoint = random.randint(0x010000, 0x10FFFF)
            string1 += chr(codepoint) # add random chr outside BMP
        for j in range(length2):
            codepoint = random.randint(0x010000, 0x10FFFF)
            string2 += chr(codepoint)
        
        assert pm.asUCS4(concatenate(string1, string2)) == (string1 + string2)

def test_eval_functions_pyfunctions_ints():
    caller = pm.eval("(func, param1, param2) => { return func(param1, param2) }")
    def add(a, b):
        return a + b
    n = 10
    for i in range(n):
        int1 = random.randint(0x0000, 0xFFFF)
        int2 = random.randint(0x0000, 0xFFFF)
        assert caller(add, int1, int2) == int1 + int2

def test_eval_functions_pyfunctions_strs():
    caller = pm.eval("(func, param1, param2) => { return func(param1, param2) }")
    def concatenate(a, b):
        return a + b
    n = 10
    for i in range(n):
        length1 = random.randint(0x0000, 0xFFFF)
        length2 = random.randint(0x0000, 0xFFFF)
        string1 = ''
        string2 = ''

        for j in range(length1):
            codepoint = random.randint(0x0000, 0xFFFF)
            string1 += chr(codepoint) # add random chr
        for j in range(length2):
            codepoint = random.randint(0x0000, 0xFFFF)
            string2 += chr(codepoint)
        assert caller(concatenate, string1, string2) == string1 + string2

def test_set_clear_timeout():
    async def async_fn():
        # standalone `setTimeout`
        loop = asyncio.get_running_loop()
        f0 = loop.create_future()
        def add(a, b, c):
          f0.set_result(a + b + c)
        pm.eval("setTimeout")(add, 0, 1, 2, 3)
        assert 6.0 == await f0

        # test `clearTimeout`
        f1 = loop.create_future()
        def to_raise(msg):
            f1.set_exception(TypeError(msg))
        timeout_id0 = pm.eval("setTimeout")(to_raise, 100, "going to be there")
        assert type(timeout_id0) == float
        assert timeout_id0 > 0                  # `setTimeout` should return a positive integer value
        assert int(timeout_id0) == timeout_id0
        with pytest.raises(TypeError, match="going to be there"):
            await f1                                # `clearTimeout` not called
        f1 = loop.create_future()
        timeout_id1 = pm.eval("setTimeout")(to_raise, 100, "shouldn't be here")
        pm.eval("clearTimeout")(timeout_id1)
        with pytest.raises(asyncio.exceptions.TimeoutError):
            await asyncio.wait_for(f1, timeout=0.5) # `clearTimeout` is called

        # `this` value in `setTimeout` callback should be the global object, as spec-ed
        assert await pm.eval("new Promise(function (resolve) { setTimeout(function(){ resolve(this == globalThis) }) })")
        # `setTimeout` should allow passing additional arguments to the callback, as spec-ed
        assert 3.0 == await pm.eval("new Promise((resolve) => setTimeout(function(){ resolve(arguments.length) }, 100, 90, 91, 92))")
        assert 92.0 == await pm.eval("new Promise((resolve) => setTimeout((...args) => { resolve(args[2]) }, 100, 90, 91, 92))")
        # TODO (Tom Tang): test `setTimeout` setting delay to 0 if < 0
        # TODO (Tom Tang): test `setTimeout` accepting string as the delay, coercing to a number like parseFloat

        # passing an invalid ID to `clearTimeout` should silently do nothing; no exception is thrown.
        pm.eval("clearTimeout(NaN)")
        pm.eval("clearTimeout(999)")
        pm.eval("clearTimeout(-1)")
        pm.eval("clearTimeout('a')")
        pm.eval("clearTimeout(undefined)")
        pm.eval("clearTimeout()")

        # should throw a TypeError when the first parameter to `setTimeout` is not a function
        with pytest.raises(pm.SpiderMonkeyError, match="TypeError: The first parameter to setTimeout\\(\\) is not a function"):
            pm.eval("setTimeout()")
        with pytest.raises(pm.SpiderMonkeyError, match="TypeError: The first parameter to setTimeout\\(\\) is not a function"):
            pm.eval("setTimeout(undefined)")
        with pytest.raises(pm.SpiderMonkeyError, match="TypeError: The first parameter to setTimeout\\(\\) is not a function"):
            pm.eval("setTimeout(1)")

        # making sure the async_fn is run
        return True
    assert asyncio.run(async_fn())

def test_promises():
    async def async_fn():
        # Python awaitables to JS Promise coercion
        # 1. Python asyncio.Future to JS promise
        loop = asyncio.get_running_loop()
        f0 = loop.create_future()
        f0.set_result(2561)
        assert type(f0) == asyncio.Future
        assert 2561 == await f0
        assert pm.eval("(p) => p instanceof Promise")(f0) is True
        assert 2561 == await pm.eval("(p) => p")(f0)
        del f0

        # 2. Python asyncio.Task to JS promise
        async def coro_fn(x):
            await asyncio.sleep(0.01)
            return x
        task = loop.create_task(coro_fn("from a Task"))
        assert type(task) == asyncio.Task
        assert type(task) != asyncio.Future
        assert isinstance(task, asyncio.Future)
        assert "from a Task" == await task
        assert pm.eval("(p) => p instanceof Promise")(task) is True
        assert "from a Task" == await pm.eval("(p) => p")(task)
        del task

        # 3. Python coroutine to JS promise
        coro = coro_fn("from a Coroutine")
        assert asyncio.iscoroutine(coro)
        # assert "a Coroutine" == await coro                            # coroutines cannot be awaited more than once
        # assert pm.eval("(p) => p instanceof Promise")(coro) is True   #   RuntimeError: cannot reuse already awaited coroutine
        assert "from a Coroutine" == await pm.eval("(p) => (p instanceof Promise) && p")(coro)
        del coro

        # JS Promise to Python awaitable coercion
        assert 100 == await pm.eval("new Promise((r)=>{ r(100) })")
        assert 10010 == await pm.eval("Promise.resolve(10010)")
        with pytest.raises(pm.SpiderMonkeyError, match="TypeError: .+ is not a constructor"):
            await pm.eval("Promise.resolve")(10086)
        assert 10086 == await pm.eval("Promise.resolve.bind(Promise)")(10086)

        assert "promise returning a function" == (await pm.eval("Promise.resolve(() => { return 'promise returning a function' })"))()
        assert "function 2" == (await pm.eval("Promise.resolve(x=>x)"))("function 2")
        def aaa(n):
            return n
        ident0 = await (pm.eval("Promise.resolve.bind(Promise)")(aaa))
        assert "from aaa" == ident0("from aaa")
        ident1 = await pm.eval("async (aaa) => x=>aaa(x)")(aaa)
        assert "from ident1" == ident1("from ident1")
        ident2 = await pm.eval("() => Promise.resolve(x=>x)")()
        assert "from ident2" == ident2("from ident2")
        ident3 = await pm.eval("(aaa) => Promise.resolve(x=>aaa(x))")(aaa)
        assert "from ident3" == ident3("from ident3")
        del aaa

        # promise returning a JS Promise<Function> that calls a Python function inside
        def fn0(n):
            return n + 100
        def fn1():
            return pm.eval("async x=>x")(fn0)
        fn2 = await pm.eval("async (fn1) => { const fn0 = await fn1(); return Promise.resolve(x=>fn0(x)) }")(fn1)
        assert 101.2 == fn2(1.2)
        fn3 = await pm.eval("async (fn1) => { const fn0 = await fn1(); return Promise.resolve(async x => { return fn0(x) }) }")(fn1)
        assert 101.3 == await fn3(1.3)
        fn4 = await pm.eval("async (fn1) => { return Promise.resolve(async x => { const fn0 = await fn1(); return fn0(x) }) }")(fn1)
        assert 101.4 == await fn4(1.4)
        
        # chained JS promises
        assert "chained" == await (pm.eval("async () => new Promise((resolve) => resolve( Promise.resolve().then(()=>'chained') ))")())

        # chained Python awaitables
        async def a():
            await asyncio.sleep(0.01)
            return "nested"
        async def b():
            await asyncio.sleep(0.01)
            return a()
        async def c():
            await asyncio.sleep(0.01)
            return b()
        # JS `await` supports chaining. However, on Python-land, it actually requires `await (await (await c()))`
        assert "nested" == await pm.eval("async (promise) => await promise")(c())
        assert "nested" == await pm.eval("async (promise) => await promise")(await c())
        assert "nested" == await pm.eval("async (promise) => await promise")(await (await c()))
        assert "nested" == await pm.eval("async (promise) => await promise")(await (await (await c())))
        assert "nested" == await pm.eval("async (promise) => promise")(c())
        assert "nested" == await pm.eval("async (promise) => promise")(await c())
        assert "nested" == await pm.eval("async (promise) => promise")(await (await c()))
        assert "nested" == await pm.eval("async (promise) => promise")(await (await (await c())))
        assert "nested" == await pm.eval("(promise) => Promise.resolve(promise)")(c())
        assert "nested" == await pm.eval("(promise) => Promise.resolve(promise)")(await c())
        assert "nested" == await pm.eval("(promise) => Promise.resolve(promise)")(await (await c()))
        assert "nested" == await pm.eval("(promise) => Promise.resolve(promise)")(await (await (await c())))
        assert "nested" == await pm.eval("(promise) => promise")(c())
        assert "nested" == await pm.eval("(promise) => promise")(await c())
        assert "nested" == await pm.eval("(promise) => promise")(await (await c()))
        with pytest.raises(TypeError, match="object str can't be used in 'await' expression"):
            await pm.eval("(promise) => promise")(await (await (await c())))

        # Python awaitable throwing exceptions
        async def coro_to_throw0():
            await asyncio.sleep(0.01)
            print([].non_exist)
        with pytest.raises(pm.SpiderMonkeyError, match="Python AttributeError: 'list' object has no attribute 'non_exist'"):
            await (pm.eval("(promise) => promise")(coro_to_throw0()))
        with pytest.raises(pm.SpiderMonkeyError, match="Python AttributeError: 'list' object has no attribute 'non_exist'"):
            await (pm.eval("async (promise) => promise")(coro_to_throw0()))
        with pytest.raises(pm.SpiderMonkeyError, match="Python AttributeError: 'list' object has no attribute 'non_exist'"):
            await (pm.eval("(promise) => Promise.resolve().then(async () => await promise)")(coro_to_throw0()))
        async def coro_to_throw1():
            await asyncio.sleep(0.01)
            raise TypeError("reason")
        with pytest.raises(pm.SpiderMonkeyError, match="Python TypeError: reason"):
            await (pm.eval("(promise) => promise")(coro_to_throw1()))
        assert 'rejected <Python TypeError: reason>' == await pm.eval("(promise) => promise.then(()=>{}, (err)=>`rejected <${err.message}>`)")(coro_to_throw1())

        # JS Promise throwing exceptions
        with pytest.raises(pm.SpiderMonkeyError, match="nan"):
            await pm.eval("Promise.reject(NaN)") # JS can throw anything
        with pytest.raises(pm.SpiderMonkeyError, match="123.0"):
            await (pm.eval("async () => { throw 123 }")())
            # await (pm.eval("async () => { throw {} }")())
        with pytest.raises(pm.SpiderMonkeyError, match="anything"):
            await pm.eval("Promise.resolve().then(()=>{ throw 'anything' })")
            # FIXME (Tom Tang): We currently handle Promise exceptions by converting the object thrown to a Python Exception object through `pyTypeFactory`
            #               <objects of this type are not handled by PythonMonkey yet>
            # await pm.eval("Promise.resolve().then(()=>{ throw {a:1,toString(){return'anything'}} })")
        with pytest.raises(pm.SpiderMonkeyError, match="on line 1:\nTypeError: undefined has no properties"): # not going through the conversion
            await pm.eval("Promise.resolve().then(()=>{ (undefined).prop })")

        # TODO (Tom Tang): Modify this testcase once we support ES2020-style dynamic import
        pm.eval("import('some_module')") # dynamic import returns a Promise, see https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/import
        with pytest.raises(pm.SpiderMonkeyError, match="\nError: Dynamic module import is disabled or not supported in this context"):
            await pm.eval("import('some_module')")

        # await scheduled jobs on the Python event-loop
        js_sleep = pm.eval("(second) => new Promise((resolve) => setTimeout(resolve, second*1000))")
        py_sleep = asyncio.sleep
        both_sleep = pm.eval("(js_sleep, py_sleep) => async (second) => { await js_sleep(second); await py_sleep(second) }")(js_sleep, py_sleep)
        await asyncio.wait_for(both_sleep(0.1), timeout=0.21)
        with pytest.raises(asyncio.exceptions.TimeoutError):
            await asyncio.wait_for(both_sleep(0.1), timeout=0.19)

        # making sure the async_fn is run
        return True
    assert asyncio.run(async_fn())
