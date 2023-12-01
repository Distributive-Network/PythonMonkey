import pythonmonkey as pm

def test_eval_lists():
    d = [1]
    proxy_d = pm.eval("(list) => { return list; }")(d)
    assert d is proxy_d

def test_equal_lists_left_literal_right():
    pyArray = pm.eval("Array(1,2)")
    assert pyArray == [1,2]

def test_equal_lists_right_literal_left():
    pyArray = pm.eval("Array(1,2)")
    assert [1,2] == pyArray

def test_equal_lists_left_list_right():
    pyArray = pm.eval("Array(1,2)")
    b = [1,2]
    assert pyArray == b

def test_equal_lists_right_literal_left():
    pyArray = pm.eval("Array(1,2)")
    b = [1,2]
    assert b == pyArray

def test_not_equal_lists_left_literal_right():
    pyArray = pm.eval("Array(1,2)")
    assert pyArray != [1,3]  

def test_smaller_lists_left_literal_right():
    pyArray = pm.eval("Array(1,2)")
    assert pyArray < [1,3]  

def test_equal_sublist():
    pyArray = pm.eval("Array(1,2,[3,4])")
    assert pyArray == [1,2,[3,4]]

def test_not_equal_sublist_right_not_sublist():
    pyArray = pm.eval("Array(1,2,[3,4])")
    assert pyArray != [1,2,3,4]   

