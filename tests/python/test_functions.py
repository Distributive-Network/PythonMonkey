import pythonmonkey as pm


def test_func_no_args():
  def f():
    return 42
  assert 42 == pm.eval("(f) => f()")(f)


def test_func_too_many_args():
  def f(a, b):
    return [a, b]
  assert [1, 2] == pm.eval("(f) => f(1, 2, 3)")(f)


def test_func_equal_args():
  def f(a, b):
    return [a, b]
  assert [1, 2] == pm.eval("(f) => f(1, 2)")(f)


def test_func_too_few_args():
  def f(a, b):
    return [a, b]
  assert [1, None] == pm.eval("(f) => f(1)")(f)


def test_default_func_no_args():
  def f(a, b, c=42, d=43):
    return [a, b, c, d]
  assert [None, None, 42, 43] == pm.eval("(f) => f()")(f)


def test_default_func_too_many_default_args():
  def f(a, b, c=42, d=43):
    return [a, b, c, d]
  assert [1, 2, 3, 4] == pm.eval("(f) => f(1, 2, 3, 4, 5)")(f)


def test_default_func_equal_default_args():
  def f(a, b, c=42, d=43):
    return [a, b, c, d]
  assert [1, 2, 3, 4] == pm.eval("(f) => f(1, 2, 3, 4)")(f)


def test_default_func_too_few_default_args():
  def f(a, b, c=42, d=43):
    return [a, b, c, d]
  assert [1, 2, 3, 43] == pm.eval("(f) => f(1, 2, 3)")(f)


def test_default_func_equal_args():
  def f(a, b, c=42, d=43):
    return [a, b, c, d]
  assert [1, 2, 42, 43] == pm.eval("(f) => f(1, 2)")(f)


def test_default_func_too_few_args():
  def f(a, b, c=42, d=43):
    return [a, b, c, d]
  assert [1, None, 42, 43] == pm.eval("(f) => f(1)")(f)


def test_vararg_func_no_args():
  def f(a, b, *args):
    return [a, b, *args]
  assert [None, None] == pm.eval("(f) => f()")(f)


def test_vararg_func_too_many_args():
  def f(a, b, *args):
    return [a, b, *args]
  assert [1, 2, 3] == pm.eval("(f) => f(1, 2, 3)")(f)


def test_vararg_func_equal_args():
  def f(a, b, *args):
    return [a, b, *args]
  assert [1, 2] == pm.eval("(f) => f(1, 2)")(f)


def test_vararg_func_too_few_args():
  def f(a, b, *args):
    return [a, b, *args]
  assert [1, None] == pm.eval("(f) => f(1)")(f)


def test_default_vararg_func_no_args():
  def f(a, b, c=42, d=43, *args):
    return [a, b, c, d, *args]
  assert [None, None, 42, 43] == pm.eval("(f) => f()")(f)


def test_default_vararg_func_too_many_default_args():
  def f(a, b, c=42, d=43, *args):
    return [a, b, c, d, *args]
  assert [1, 2, 3, 4, 5] == pm.eval("(f) => f(1, 2, 3, 4, 5)")(f)


def test_default_vararg_func_equal_default_args():
  def f(a, b, c=42, d=43, *args):
    return [a, b, c, d, *args]
  assert [1, 2, 3, 4] == pm.eval("(f) => f(1, 2, 3, 4)")(f)


def test_default_vararg_func_too_few_default_args():
  def f(a, b, c=42, d=43, *args):
    return [a, b, c, d, *args]
  assert [1, 2, 3, 43] == pm.eval("(f) => f(1, 2, 3)")(f)


def test_default_vararg_func_equal_args():
  def f(a, b, c=42, d=43, *args):
    return [a, b, c, d, *args]
  assert [1, 2, 42, 43] == pm.eval("(f) => f(1, 2)")(f)


def test_default_vararg_func_too_few_args():
  def f(a, b, c=42, d=43, *args):
    return [a, b, c, d, *args]
  assert [1, None, 42, 43] == pm.eval("(f) => f(1)")(f)

def test_kwargs_in_js():
  assert [1, 2, {"i": 42, "j": 43}] == pm.eval("(first, second, kwargs) => {return [first, second, kwargs]}")(1, 2, i=42, j=43)

def test_kwargs_in_js_arguments_array():
  # Note: arguments object is only available in non-arrow functions
  assert [1, 2, {"i": 42, "j": 43}] == pm.eval("(function() {return [...arguments]})")(1, 2, i=42, j=43)

def test_kwargs_pass_through():
  def py_func(*args, **kwargs):
    return [*args, kwargs];
  
  # Note: kwargs passed py1->js->py2 end up in args for py2 rather than kwargs, since JS has no equivalent to kwargs
  assert [1, 2, {"i": 42, "j": 43}, {}] == pm.eval("(py_func, ...args) => { return py_func(...args)}")(py_func, 1, 2, i=42, j=43)