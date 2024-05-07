import pythonmonkey as pm

# get


def test_get_no_default():
  likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
  foundKey = likes.get('fruit')
  assert foundKey == 'apple'


def test_get_no_default_not_found():
  likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
  foundKey = likes.get('fuit')
  assert foundKey is None


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
  assert likes['colo'] is None
  assert foundKey is None


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


def test_setdefault_with_shadowing():
  jsObj = pm.eval("({get: 'value'})")
  a = jsObj.setdefault("get", "val")
  assert a == 'value'

# pop


def test_pop_found():
  likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
  foundKey = likes.pop('color')
  assert likes['color'] is None
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

# clear


def test_clear():
  likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
  likes.clear()
  assert len(likes) == 0

# copy


def test_copy():
  likes = pm.eval('({"color": "blue", "fruit": "apple", "pet": "dog"})')
  otherLikes = likes.copy()
  otherLikes["color"] = "yellow"

  assert likes == {"color": "blue", "fruit": "apple", "pet": "dog"}
  assert otherLikes == {"color": "yellow", "fruit": "apple", "pet": "dog"}

# update


def test_update_true_dict_right():
  a = pm.eval("({'c':5})")
  b = {'d': 6.0}
  a.update(b)
  assert a == {'c': 5.0, 'd': 6.0}
  assert b == {'d': 6.0}


def test_update_true_dict_left():
  a = {'d': 6.0}
  b = pm.eval("({'c':5})")
  a.update(b)
  assert a == {'c': 5.0, 'd': 6.0}
  assert b == {'c': 5.0}


def test_update_true_two_pm_dicts():
  a = pm.eval("({'c':5})")
  b = pm.eval("({'d':6})")
  a.update(b)
  assert a == {'c': 5.0, 'd': 6.0}
  assert b == {'d': 6.0}


def test_update_two_args():
  a = pm.eval("({'c':5})")
  a.update(B='For', C='Geeks')
  assert a == {'c': 5.0, 'B': 'For', 'C': 'Geeks'}


def test_update_iterable():
  car = pm.eval('({"brand": "Ford","model": "Mustang","year": 1964})')
  car.update([('y', 3), ('z', 0)])
  assert car == {'brand': 'Ford', 'model': 'Mustang', 'year': 1964, 'y': 3, 'z': 0}


def test_update_iterable_wrong_type():
  car = pm.eval('({"brand": "Ford","model": "Mustang","year": 1964})')
  a = [1, 2]
  try:
    car.update(a)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "cannot convert dictionary update sequence element #0 to a sequence"

# keys


def test_keys_iter():
  obj = pm.eval("({ a: 123, b: 'test' })")
  result = []
  for i in obj.keys():
    result.append(i)
  assert result == ['a', 'b']


def test_keys_iter_reverse():
  obj = pm.eval("({ a: 123, b: 'test' })")
  result = []
  for i in reversed(obj.keys()):
    result.append(i)
  assert result == ['b', 'a']


def test_keys_list():
  obj = pm.eval("({ a: 123, b: 'test' })")
  assert list(obj.keys()) == ['a', 'b']


def test_keys_repr():
  car = pm.eval('({"brand": "Ford","model": "Mustang","year": 1964})')
  a = car.keys()
  assert str(a) == "dict_keys(['brand', 'model', 'year'])"


def test_keys_substract():
  car = pm.eval('({"brand": "Ford","model": "Mustang","year": 1964})')
  a = car.keys()
  b = a - ['brand']
  assert b == {'model', 'year'}


def test_keys_richcompare_two_own():
  car = pm.eval('({"brand": "Ford","model": "Mustang","year": 1964})')
  a = car.keys()
  b = car.keys()
  assert a == b
  assert a is not b
  assert a <= b
  assert not (a < b)
  assert a >= b
  assert not (a > b)


def test_keys_richcompare_one_own():
  car = pm.eval('({"brand": "Ford","model": "Mustang","year": 1964})')
  a = car.keys()
  care = {"brand": "Ford", "model": "Mustang", "year": 1964}
  b = care.keys()
  assert a == b
  assert b == a
  assert a <= b
  assert not (a < b)
  assert a >= b
  assert not (a > b)
  assert b <= a
  assert not (b > a)
  assert b >= a
  assert not (b > a)


def test_keys_intersect_one_own_smaller():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  b = keys & {'eggs', 'bacon', 'salad', 'jam'}
  c = {'eggs', 'bacon', 'salad', 'jam'} & keys
  assert b == {'bacon', 'eggs'} == c


def test_keys_intersect_two_own_smaller():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  dishes1 = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1})")
  keys1 = dishes1.keys()
  b = keys & keys1
  c = keys1 & keys
  assert b == {'bacon', 'eggs', 'sausage'} == c


def test_keys_intersect_one_own():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  dishes1 = {'eggs': 2, 'sausage': 1, 'bacon': 1, 'peas': 6}
  keys1 = dishes1.keys()
  b = keys & keys1
  c = keys1 & keys
  assert b == {'bacon', 'eggs', 'sausage'} == c


def test_keys_or():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  others = keys | ['juice', 'juice', 'juice']
  assert others == {'bacon', 'spam', 'juice', 'sausage', 'eggs'}
  others = ['apple'] | keys
  assert others == {'bacon', 'spam', 'sausage', 'apple', 'eggs'}


def test_keys_xor():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  others = keys ^ {'sausage', 'juice'}
  assert others == {'bacon', 'spam', 'juice', 'eggs'}


def test_keys_len():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  assert len(keys) == 4


def test_keys_contains():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  assert 'eggs' in keys
  assert 'egg' not in keys


def test_keys_isdisjoint_self():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  assert not keys.isdisjoint(keys)


def test_keys_isdisjoint_true_keys():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  assert keys.isdisjoint({"egg": 4, "e": 5, "f": 6}.keys())


def test_keys_isnotdisjoint_true_keys():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  assert not keys.isdisjoint({"eggs": 4, "e": 5, "f": 6}.keys())


def test_keys_isnotdisjoint_own_keys():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  dishese = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys1 = dishese.keys()
  assert not keys.isdisjoint(keys1)


def test_keys_isnotdisjoint_own_keys_longer():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  dishese = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500, 'juice':6467})")
  keys1 = dishese.keys()
  assert not keys.isdisjoint(keys1)


def test_keys_isnotdisjoint_true_keys_longer():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  dishese = {'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500, 'juice': 6467}
  keys1 = dishese.keys()
  assert not keys.isdisjoint(keys1)


def test_keys_update_object_updates_the_keys():
  employee = pm.eval("({'name': 'Phil', 'age': 22})")
  dictionaryKeys = employee.keys()
  employee.update({'salary': 3500.0})
  assert str(dictionaryKeys) == "dict_keys(['name', 'age', 'salary'])"


def test_keys_mapping():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  keys = dishes.keys()
  assert str(keys.mapping) == "{'eggs': 2.0, 'sausage': 1.0, 'bacon': 1.0, 'spam': 500.0}"
  assert keys.mapping['spam'] == 500

# values


def test_values_iter():
  obj = pm.eval("({ a: 123, b: 'test' })")
  result = []
  for i in obj.values():
    result.append(i)
  assert result == [123, 'test']


def test_values_iter_reversed():
  obj = pm.eval("({ a: 123, b: 'test' })")
  result = []
  for i in reversed(obj.values()):
    result.append(i)
  assert result == ['test', 123]


def test_values_list():
  obj = pm.eval("({ a: 123, b: 'test' })")
  assert list(obj.values()) == [123, 'test']


def test_values_repr():
  car = pm.eval('({"brand": "Ford","model": "Mustang","year": 1964})')
  a = car.values()
  assert str(a) == "dict_values(['Ford', 'Mustang', 1964.0])"


def test_values_update_object_updates_the_values():
  employee = pm.eval("({'name': 'Phil', 'age': 22})")
  dictionaryValues = employee.values()
  employee.update({'salary': 3500.0})
  assert str(dictionaryValues) == "dict_values(['Phil', 22.0, 3500.0])"


def test_values_mapping():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  values = dishes.values()
  assert str(values.mapping) == "{'eggs': 2.0, 'sausage': 1.0, 'bacon': 1.0, 'spam': 500.0}"
  assert values.mapping['spam'] == 500

# items


def test_items_iter():
  obj = pm.eval("({ a: 123, b: 'test' })")
  result = []
  for i in obj.items():
    result.append(i)
  assert result == [('a', 123.0), ('b', 'test')]


def test_items_iter_reversed():
  obj = pm.eval("({ a: 123, b: 'test' })")
  result = []
  for i in reversed(obj.items()):
    result.append(i)
  assert result == [('b', 'test'), ('a', 123.0)]


def test_items_list():
  obj = pm.eval("({ a: 123, b: 'test' })")
  assert list(obj.items()) == [('a', 123.0), ('b', 'test')]


def test_items_repr():
  car = pm.eval('({"brand": "Ford","model": "Mustang","year": 1964})')
  a = car.items()
  assert str(a) == "dict_items([('brand', 'Ford'), ('model', 'Mustang'), ('year', 1964.0)])"


def test_items_substract():
  car = pm.eval('({"brand": "Ford","model": "Mustang","year": 1964})')
  a = car.items()
  b = a - [('brand', 'Ford')]
  assert b == {('model', 'Mustang'), ('year', 1964.0)}


def test_items_or():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  items = dishes.items()
  others = items | [('juice', 'apple')]
  assert others == {('spam', 500.0), ('juice', 'apple'), ('eggs', 2.0), ('sausage', 1.0), ('bacon', 1.0)}
  others = [('juice', 'raisin')] | items
  assert others == {('spam', 500.0), ('juice', 'raisin'), ('eggs', 2.0), ('sausage', 1.0), ('bacon', 1.0)}


def test_items_xor():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  items = dishes.items()
  others = items ^ {('eggs', 2)}
  assert others == {('spam', 500), ('bacon', 1), ('sausage', 1)}


def test_items_intersect_one_own_smaller():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  items = dishes.items()
  b = items & {('eggs', 2), ('bacon', 1), ('salad', 12), ('jam', 34)}
  c = {('eggs', 2), ('bacon', 1), ('salad', 12), ('jam', 34)} & items
  assert b == {('bacon', 1), ('eggs', 2)} == c


def test_items_intersect_one():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  items = dishes.items()
  b = items & {('eggs', 2)}
  assert b == {('eggs', 2)}


def test_items_intersect_none():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  items = dishes.items()
  b = items & {('ketchup', 12)}
  assert str(b) == "set()"


def test_items_len():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  items = dishes.items()
  assert len(items) == 4

# TODO tuple support
# def test_items_contains():
#    dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
#    items = dishes.items()
#    assert {('eggs', 2)} in items


def test_items_update_object_updates_the_items():
  employee = pm.eval("({'name': 'Phil', 'age': 22})")
  dictionaryItems = employee.items()
  employee.update({('salary', 3500.0)})
  assert str(dictionaryItems) == "dict_items([('name', 'Phil'), ('age', 22.0), ('salary', 3500.0)])"


def test_items_mapping():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  items = dishes.items()
  assert str(items.mapping) == "{'eggs': 2.0, 'sausage': 1.0, 'bacon': 1.0, 'spam': 500.0}"
  assert items.mapping['spam'] == 500

# get method


def test_get_method():
  dishes = pm.eval("({'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500})")
  assert dishes.get('eggs') == 2

# get method shadowing


def test_method_shadowing():
  jsObj = pm.eval("({get: 'value'})")
  assert repr(jsObj.get).__contains__("<built-in method get of dict object at")
  assert jsObj['get'] == 'value'


# next operator
def test_next_operator():
  myit = pm.eval('function* makeIt() { yield 1; yield 2; }; makeIt')()
  first = next(myit)
  assert (first == 1.0)
  second = next(myit)
  assert (second == 2.0)
  try:
    third = next(myit)
    assert (False)
  except StopIteration as e:
    assert (True)
  fourth = next(myit, 'default')
  assert fourth == 'default'
