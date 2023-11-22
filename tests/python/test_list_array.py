import pythonmonkey as pm

def test_eval_list_is_array():
    items = [1, 2, 3]
    isArray = pm.eval('Array.isArray')(items)
    assert isArray == True

def test_eval_array_is_list():
    pythonList = pm.eval('[]')
    assert isinstance(pythonList, list) 

# extra nice but not necessary
def test_eval_array_is_list_type_string():
    pythonListTypeString = str(type(pm.eval('[]')))
    assert pythonListTypeString == "<class 'list'>"
