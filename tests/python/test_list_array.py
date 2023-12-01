import pythonmonkey as pm

def test_eval_array_is_list():
    pythonList = pm.eval('[]')
    assert isinstance(pythonList, list) 

# extra nice but not necessary
def test_eval_array_is_list_type_string():
    pythonListTypeString = str(type(pm.eval('[]')))
    assert pythonListTypeString == "<class 'pythonmonkey.JSArrayProxy'>"
