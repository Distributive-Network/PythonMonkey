import pytest
import pythonmonkey as pm
import random
from datetime import datetime, timedelta, timezone
import math

def test_passes():
    assert True

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
    start = datetime(MIN_YEAR, 1, 1, 00, 00, 00, tzinfo=timezone.utc)
    years = MAX_YEAR - MIN_YEAR + 1
    end = start + timedelta(days=365 * years)
    for _ in range(10):
        py_date = start + (end - start) * random.random()
        # round to milliseconds precision because the smallest unit for js Date is 1ms
        py_date = py_date.replace(microsecond=min(round(py_date.microsecond, -3), 999000)) # microsecond must be in 0..999999, but it would be rounded to 1000000 if >= 999500
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

def test_eval_exceptions_nested_py_js_py():
    def c():
        raise Exception('this is an exception')
    b = pm.eval('''(x) => {
        try { 
            x() 
        } catch(e) {
            return "Caught in JS " + e;
        }
    }''')
    assert b(c) == "Caught in JS Error: Python Exception: this is an exception"

def test_eval_exceptions_nested_js_py_js():
    c = pm.eval("() => { throw TypeError('this is an exception'); }")

    def b(x):
        try:
            x()
            return ""
        except Exception as e:
            return "Caught in Py " + str(e)
    ret = b(c)
    assert ("Caught in Py Error in" in ret) and ("TypeError: this is an exception" in ret)

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
        
        assert concatenate(string1, string2) == (string1 + string2)

def test_eval_functions_roundtrip():
    # BF-60 https://github.com/Distributive-Network/PythonMonkey/pull/18
    def ident(x):
        return x
    js_fn_back = pm.eval("(py_fn) => py_fn(()=>{ return 'YYZ' })")(ident)
    # pm.collect() # TODO: to be fixed in BF-59
    assert "YYZ" == js_fn_back()

def test_eval_functions_pyfunction_in_closure():
    # BF-58 https://github.com/Distributive-Network/PythonMonkey/pull/19
    def fn1():
        def fn0(n):
            return n + 100
        return fn0
    assert 101.9 == fn1()(1.9)
    assert 101.9 == pm.eval("(fn1) => { return fn1 }")(fn1())(1.9)
    assert 101.9 == pm.eval("(fn1, x) => { return fn1()(x) }")(fn1, 1.9)
    assert 101.9 == pm.eval("(fn1) => { return fn1() }")(fn1)(1.9)

def test_unwrap_py_function():
    # https://github.com/Distributive-Network/PythonMonkey/issues/65
    def pyFunc():
        pass
    unwrappedPyFunc = pm.eval("(wrappedPyFunc) => { return wrappedPyFunc }")(pyFunc)
    assert unwrappedPyFunc is pyFunc

def test_unwrap_js_function():
    # https://github.com/Distributive-Network/PythonMonkey/issues/65
    wrappedJSFunc = pm.eval("const JSFunc = () => { return 0 }\nJSFunc")
    assert pm.eval("(unwrappedJSFunc) => { return unwrappedJSFunc === JSFunc }")(wrappedJSFunc)

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
