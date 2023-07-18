import pytest
import pythonmonkey as pm

def test_function_python_unwrapping():
  def pyFunc():
    return 0
  unwrappedPyFunc = pm.eval("(wrappedPyFunc) => wrappedPyFunc;")(pyFunc)
  assert pyFunc is unwrappedPyFunc

def test_function_js_unwrapping():
  wrappedJSFunc = pm.eval("let JSFunc = () => 0;\nJSFunc")
  assert pm.eval("(unwrappedJSFunc) => unwrappedJSFunc === JSFunc;")(wrappedJSFunc)

def test_function_python_attributes():
  def pyFunc():
    return 0
  pyFunc.attr1 = "This is an attribute"
  assert pm.eval("(pyFunc) => pyFunc.attr1")(pyFunc) == "This is an attribute"
  pm.eval("(pyFunc) => { pyFunc.attr2 = 'This is another attribute'; }")(pyFunc)
  assert pyFunc.attr2 == "This is another attribute"

def test_function_js_props():
  jsFunc = pm.eval("let jsFunc = () => 0;\njsFunc")
  pm.eval("jsFunc.prop1 = 'This is a prop';")
  assert jsFunc.prop1 == "This is a prop"
  jsFunc.prop2 = "This is another prop"
  assert pm.eval("jsFunc.prop2 === 'This is another prop'")