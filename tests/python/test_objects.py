import pythonmonkey as pm


def test_eval_pyobjects():
  class MyClass:
    pass

  o = MyClass()
  proxy_o = pm.eval("(obj) => { return obj; }")(o)
  assert o is proxy_o


def test_eval_pyobjects_subobjects():
  class InnerClass:
    def __init__(self):
      self.c = 2

  class OuterClass:
    def __init__(self):
      self.a = 1
      self.b = InnerClass()

  o = OuterClass()

  assert pm.eval("(obj) => { return obj.a;   }")(o) == 1.0
  assert pm.eval("(obj) => { return obj.b;   }")(o) is o.b
  assert pm.eval("(obj) => { return obj.b.c; }")(o) == 2.0


def test_eval_pyobjects_cycle():
  class MyClass:
    def __init__(self):
      self.a = 1
      self.b = 2
      self.recursive = self

  o = MyClass()

  assert pm.eval("(obj) => { return obj.a;         }")(o) == 1.0
  assert pm.eval("(obj) => { return obj.b;         }")(o) == 2.0
  assert pm.eval("(obj) => { return obj.recursive; }")(o) is o.recursive


def test_eval_pyobjects_proxy_get():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()

  assert pm.eval("(obj) => { return obj.a}")(o) == 42.0


def test_eval_pyobjects_proxy_set():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()

  pm.eval("(obj) => { obj.b = 43; }")(o)
  assert o.b == 43.0


def test_eval_pyobjects_proxy_keys():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()

  assert pm.eval("(obj) => { return Object.keys(obj)[0]; }")(o) == 'a'


def test_eval_pyobjects_proxy_delete():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()

  pm.eval("(obj) => { delete obj.a; }")(o)
  assert not hasattr(o, 'a')


def test_eval_pyobjects_proxy_has():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()

  assert pm.eval("(obj) => { return 'a' in obj; }")(o)


def test_eval_pyobjects_proxy_not_extensible():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()

  assert not pm.eval("(o) => Object.isExtensible(o)")(o)
  assert pm.eval("(o) => Object.preventExtensions(o) === o")(o)


def test_instanceof_pyobject():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()
  assert pm.eval("(obj) => { return obj instanceof Object; }")(o)


def test_pyobjects_not_instanceof_string():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()

  assert not pm.eval("(obj) => { return obj instanceof String; }")(o)


def test_pyobjects_valueOf():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()
  assert o is pm.eval("(obj) => { return obj.valueOf(); }")(o)


def test_pyobjects_toString():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()
  assert '[object Object]' == pm.eval("(obj) => { return obj.toString(); }")(o)


def test_pyobjects_toLocaleString():
  class MyClass:
    def __init__(self):
      self.a = 42

  o = MyClass()
  assert '[object Object]' == pm.eval("(obj) => { return obj.toLocaleString(); }")(o)
