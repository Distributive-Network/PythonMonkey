import pythonmonkey as pm
import subprocess

def test_python_functions_self():
  def pyFunc(param):
    return param
  
  assert 1 == pyFunc(1)
  assert 2 == pm.eval("""(pyFunc) => {
    return pyFunc(2);
  }
  """)(pyFunc)
  assert 3 == pm.eval("""(pyFunc) => {
    let jsObj = {};
    jsObj.pyFunc = pyFunc;
    return pyFunc(3);
  }
  """)(pyFunc)

def test_python_methods_self():
  def pyFunc(self, param):
    return [self, param]

  class Class:
    pass
  Class.pyMethod = pyFunc

  pyObj = Class()
  result = pyObj.pyMethod(1)
  assert pyObj == result[0] and 1 == result[1]
  pyMethod = pyObj.pyMethod
  result = pyMethod(2)
  assert pyObj == result[0] and 2 == result[1]
  result = pm.eval("""(pyObj) => {
    return pyObj.pyMethod(3);
  }
  """)(pyObj)
  assert pyObj == result[0] and 3 == result[1]
  result = pm.eval("""(pyObj) => {
    let jsObj = {};
    jsObj.pyMethod = pyObj.pyMethod;
    return jsObj.pyMethod(4);
  }
  """)(pyObj)
  assert pyObj == result[0] and 4 == result[1] #pyMethod is bound to pyObj, so `self` is not `jsObj`
  result = pm.eval("""(pyObj) => {
    let pyMethod = pyObj.pyMethod;
    return pyMethod(5);
  }
  """)(pyObj)
  assert pyObj == result[0] and 5 == result[1] #pyMethod is bound to pyObj, so `self` is not `globalThis`

def test_javascript_functions_this():
  jsFunc = pm.eval("""
  (function(param) {
    return [this, param];
  })
  """)

  class Class:
    pass
  Class.jsFunc = jsFunc # jsFunc is not bound to Class, so `this` will be `globalThis`, not the object
  
  pyObj = Class()
  jsObj = pm.eval("({})")
  jsObj.jsFunc = jsFunc
  globalThis = pm.eval("globalThis")
  result = jsFunc(1)
  assert globalThis == result[0] and 1 == result[1]
  result = pyObj.jsFunc(2)
  assert globalThis == result[0] and 2 == result[1]
  result = jsObj.jsFunc(3)
  assert globalThis == result[0] and 3 == result[1] # TODO (Caleb Aikens) should `this` be `globalThis` or `jsObj` here?
  result = pm.eval("""(jsFunc) => {
    return jsFunc(4);
  }
  """)(jsFunc)
  assert globalThis == result[0] and 4 == result[1]
  result = pm.eval("""(pyObj) => {
    return pyObj.jsFunc(5);
  }
  """)(pyObj)
  assert pyObj == result[0] and 5 == result[1]
  result = pm.eval("""(jsObj) => {
    return jsObj.jsFunc(6);
  }
  """)(jsObj)
  assert jsObj == result[0] and 6 == result[1]

def test_JSMethodProxy_this():
  jsFunc = pm.eval("""
  (function(param) {
    return [this, param];
  })
  """)

  class Class:
    pass
  
  pyObj = Class()
  pyObj.jsMethod = pm.JSMethodProxy(jsFunc, pyObj) # jsMethod is bound to pyObj, so `this` will always be `pyObj`
  jsObj = pm.eval("({})")
  jsObj.jsMethod = pyObj.jsMethod
  globalThis = pm.eval("globalThis")
  result = pyObj.jsMethod(1)
  assert pyObj == result[0] and 1 == result[1]
  result = jsObj.jsMethod(2)
  assert pyObj == result[0] and 2 == result[1]
  result = pm.eval("""(pyObj) => {
    return pyObj.jsMethod(3);
  }
  """)(pyObj)
  assert pyObj == result[0] and 3 == result[1]
  result = pm.eval("""(jsObj) => {
    return jsObj.jsMethod(4);
  }
  """)(jsObj)
  assert pyObj == result[0] and 4 == result[1]  #TODO (Caleb Aikens) should `this` be `pyObj` or `jsObj` here?

#require
def test_require_correct_this():
    subprocess.check_call('npm install crypto-js', shell=True)
    CryptoJS = pm.require('crypto-js')
    cipher = CryptoJS.SHA256("Hello, World!").toString()
    assert cipher == "dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f"