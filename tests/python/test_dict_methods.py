import pythonmonkey as pm

# get
def test_get_no_default():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    foundKey = likes.get('fruit')
    assert foundKey == 'apple'

def test_get_no_default_not_found():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    foundKey = likes.get('fuit')
    assert foundKey == None

def test_get_default_not_found():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    foundKey = likes.get('fuit', 'orange')
    assert foundKey == 'orange'

def test_get_no_params():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    try:
        likes.get()           
        assert (False)
    except Exception as e:   
        assert str(type(e)) == "<class 'TypeError'>"
        assert str(e) == "get expected at least 1 argument, got 0"

# setdefault
def test_setdefault_found():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    foundKey = likes.setdefault('color')
    assert foundKey == 'blue'

def test_setdefault_found_ignore_default():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    foundKey = likes.setdefault('color', 'yello')
    assert foundKey == 'blue'

def test_setdefault_not_found_no_default():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    foundKey = likes.setdefault('colo')
    assert likes['colo'] == None
    assert foundKey == None    

def test_setdefault_not_found_with_default():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    foundKey = likes.setdefault('colo', 'yello')
    assert likes['colo'] == 'yello'
    assert foundKey == 'yello'  

def test_setdefault_no_params():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    try:
        likes.setdefault()           
        assert (False)
    except Exception as e:    
        assert str(type(e)) == "<class 'TypeError'>"
        assert str(e) == "setdefault expected at least 1 argument, got 0"      

#pop
def test_pop_found():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    foundKey = likes.pop('color')
    assert likes['color'] == None
    assert foundKey == 'blue'

def test_pop_not_found():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    try:
        likes.pop('colo')          
        assert (False)
    except Exception as e:    
        assert str(type(e)) == "<class 'KeyError'>"
        assert str(e) == "'colo'"   

def test_pop_twice_not_found():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    likes.pop('color')
    try:
        likes.pop('color')          
        assert (False)
    except Exception as e:    
        assert str(type(e)) == "<class 'KeyError'>"
        assert str(e) == "'color'"         

def test_pop_found_ignore_default():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    foundKey = likes.pop('color', 'u')
    assert foundKey == 'blue'

def test_pop_not_found_with_default():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    foundKey = likes.pop('colo', 'unameit')
    assert foundKey == 'unameit'

def test_pop_not_found_no_default():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    try:
        likes.pop('colo')           
        assert (False)
    except Exception as e:    
        assert str(type(e)) == "<class 'KeyError'>"
        assert str(e) == "'colo'"        

def test_pop_no_params():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    try:
        likes.pop()           
        assert (False)
    except Exception as e:    
        assert str(type(e)) == "<class 'TypeError'>"
        assert str(e) == "pop expected at least 1 argument, got 0"      

#clear
def test_clear():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    likes.clear()
    assert len(likes) == 0

#copy
def test_copy():
    likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
    otherLikes = likes.copy()
    otherLikes["color"] = "yellow"

    assert likes == {"color": "blue", "fruit": "apple", "pet": "dog"}
    assert otherLikes == {"color": "yellow", "fruit": "apple", "pet": "dog"}
