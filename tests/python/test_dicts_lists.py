import pythonmonkey as pm

def test_eval_objects():
    pyObj = pm.eval("Object({a:1.0})")
    assert pyObj == {'a':1.0}

def test_eval_objects_subobjects():
    pyObj = pm.eval("Object({a:1.0, b:{c:2.0}})")

    assert pyObj['a'] == 1.0
    assert pyObj['b'] == {'c': 2.0}
    assert pyObj['b']['c'] == 2.0

def test_eval_objects_cycle():
    pyObj = pm.eval("Object({a:1.0, b:2.0, recursive: function() { this.recursive = this; return this; }}.recursive())")
    
    assert pyObj['a'] == 1.0
    assert pyObj['b'] == 2.0
    assert pyObj['recursive'] == pyObj

def test_eval_objects_proxy_get():
    f = pm.eval("(obj) => { return obj.a}")
    assert f({'a':42.0}) == 42.0

def test_eval_objects_proxy_set():
    f = pm.eval("(obj) => { obj.a = 42.0; return;}")
    pyObj = {}
    f(pyObj)
    assert pyObj['a'] == 42.0
    
def test_eval_objects_proxy_keys():
    f = pm.eval("(obj) => { return Object.keys(obj)[0]}")
    assert f({'a':42.0}) == 'a'

def test_eval_objects_proxy_delete():
    f = pm.eval("(obj) => { delete obj.a }")
    pyObj = {'a': 42.0}
    f(pyObj)
    assert 'a' not in pyObj

def test_eval_objects_proxy_has():
    f = pm.eval("(obj) => { return 'a' in obj }")
    pyObj = {'a': 42.0}
    assert(f(pyObj))

def test_eval_objects_proxy_not_extensible():
    assert False == pm.eval("(o) => Object.isExtensible(o)")({})
    assert False == pm.eval("(o) => Object.isExtensible(o)")({ "abc": 1 })
    assert True == pm.eval("(o) => Object.preventExtensions(o) === o")({})
    assert True == pm.eval("(o) => Object.preventExtensions(o) === o")({ "abc": 1 })

def test_eval_objects_proxy_proto():
    assert pm.null == pm.eval("(o) => Object.getPrototypeOf(o)")({})
    assert pm.null == pm.eval("(o) => Object.getPrototypeOf(o)")({ "abc": 1 })

def test_eval_objects_jsproxy_get():
    proxy = pm.eval("({a: 1})")
    assert 1.0 == proxy['a']
    assert 1.0 == proxy.a

def test_eval_objects_jsproxy_set():
    proxy = pm.eval("({a: 1})")
    proxy.a = 2.0
    assert 2.0 == proxy['a']
    proxy['a'] = 3.0
    assert 3.0 == proxy.a
    proxy.b = 1.0
    assert 1.0 == proxy['b']
    proxy['b'] = 2.0
    assert 2.0 == proxy.b

def test_eval_objects_jsproxy_length():
    proxy = pm.eval("({a: 1, b:2})")
    assert 2 == len(proxy)

def test_eval_objects_jsproxy_delete():
    proxy = pm.eval("({a: 1})")
    del proxy.a
    assert None == proxy.a
    assert None == proxy['a']

def test_eval_objects_jsproxy_compare():
    proxy = pm.eval("({a: 1, b:2})")
    assert proxy == {'a': 1.0, 'b': 2.0}
