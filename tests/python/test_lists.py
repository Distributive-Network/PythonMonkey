import pythonmonkey as pm
from functools import reduce


def test_eval_lists():
  d = [1]
  proxy_d = pm.eval("(list) => { return list; }")(d)
  assert d is proxy_d


def test_equal_lists_left_literal_right():
  pyArray = pm.eval("Array(1,2)")
  assert pyArray == [1, 2]


def test_equal_no_object_lists_left_literal_right():
  pyArray = pm.eval("[1,2]")
  assert pyArray == [1, 2]


def test_equal_lists_right_literal_left():
  pyArray = pm.eval("Array(1,2)")
  assert [1, 2] == pyArray


def test_equal_lists_left_list_right():
  pyArray = pm.eval("Array(1,2)")
  b = [1, 2]
  assert pyArray == b


def test_equal_lists_right_literal_left():
  pyArray = pm.eval("Array(1,2)")
  b = [1, 2]
  assert b == pyArray


def test_not_equal_lists_left_literal_right():
  pyArray = pm.eval("Array(1,2)")
  assert pyArray != [1, 3]


def test_smaller_lists_left_literal_right():
  pyArray = pm.eval("Array(1,2)")
  assert pyArray < [1, 3]


def test_equal_sublist():
  pyArray = pm.eval("Array(1,2,[3,4])")
  assert pyArray == [1, 2, [3, 4]]


def test_not_equal_sublist_right_not_sublist():
  pyArray = pm.eval("Array(1,2,[3,4])")
  assert pyArray != [1, 2, 3, 4]


def test_equal_self():
  a = pm.eval("([1,2,3,4])")
  b = a
  assert b == a


def test_equal_other_instance():
  a = pm.eval("([1,2,3,4])")
  b = [1, 2, 3, 4]
  assert b == a


def test_not_equal_size():
  a = pm.eval("([1,2,3,4])")
  b = [1, 2]
  assert b != a


def test_equal_other_js_instance():
  a = pm.eval("([1,2,3,4])")
  b = pm.eval("([1,2,3,4])")
  assert b == a


def test_not_equal_other_js_instance():
  a = pm.eval("([1,2,3,4])")
  b = pm.eval("([1,2,4,3])")
  assert b != a


def test_not_smaller_self():
  a = pm.eval("([1,2,3,4])")
  b = a
  assert not (b < a)


def test_not_smaller_equal_self():
  a = pm.eval("([1,2,3,4])")
  b = a
  assert b <= a


def test_eval_arrays_proxy_get():
  f = pm.eval("(arr) => { return arr[0]}")
  assert f([1, 2]) == 1.0


def test_eval_arrays_proxy_set():
  f = pm.eval("(arr) => { arr[0] = 42.0; return;}")
  pyObj = [1]
  f(pyObj)
  assert pyObj[0] == 42.0

# get


def test_get():
  pyArray = pm.eval("[1,2]")
  assert pyArray[0] == 1


def test_get_negative_index():
  pyArray = pm.eval("[1,2]")
  assert pyArray[-1] == 2


def test_get_index_out_of_range():
  pyArray = pm.eval("[1,2]")
  try:
    pyArray[3]
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'IndexError'>"
    assert str(e) == 'list index out of range'


def test_get_index_wrong_type_str():
  pyArray = pm.eval("[1,2]")
  try:
    pyArray["g"]
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == 'list indices must be integers or slices, not str'


def test_get_index_wrong_type_list():
  pyArray = pm.eval("[1,2]")
  try:
    pyArray[[2]]
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == 'list indices must be integers or slices, not list'


def test_delete():
  pyArray = pm.eval("[1,2]")
  del pyArray[0]
  assert pyArray == [None, 2]


# assign
def test_assign():
  pyArray = pm.eval("[1,2]")
  pyArray[0] = 6
  assert pyArray[0] == 6


def test_assign_negative_index():
  pyArray = pm.eval("[1,2]")
  pyArray[-1] = 6
  assert pyArray[1] == 6


def test_assign_index_out_of_range():
  pyArray = pm.eval("[1,2]")
  try:
    pyArray[3] = 8
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'IndexError'>"
    assert str(e) == 'list assignment index out of range'


def test_assign_index_wrong_type_str():
  pyArray = pm.eval("[1,2]")
  try:
    pyArray["g"] = 4
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == 'list indices must be integers or slices, not str'


def test_assign_index_wrong_type_list():
  pyArray = pm.eval("[1,2]")
  try:
    pyArray[[2]] = 9
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == 'list indices must be integers or slices, not list'


# repr
def test_repr_empty_array():
  pyArray = pm.eval("[]")
  expected = "[]"
  assert repr(pyArray) == expected
  assert str(pyArray) == expected


def test_repr_non_empty_array():
  pyArray = pm.eval("[1,2]")
  expected = "[1.0, 2.0]"
  assert repr(pyArray) == expected
  assert str(pyArray) == expected


def test_repr_recursion():
  pyArray = pm.eval("""
    () => {
        let arr = [1,2];
        arr.push(arr);
        return arr;
    }
    """)()
  expected = "[1.0, 2.0, [...]]"
  assert repr(pyArray) == expected
  assert str(pyArray) == expected

# concat


def test_concat_wrong_type():
  likes = pm.eval('([1,2])')
  try:
    likes + 3
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == 'can only concatenate list (not "int") to list'


def test_concat_empty_empty():
  pyArray = pm.eval("[]")
  pyArray + []
  assert pyArray == []


def test_concat_not_empty_empty():
  pyArray = pm.eval("[1,2]")
  pyArray + []
  assert pyArray == [1, 2]


def test_concat_not_in_place():
  pyArray = pm.eval("[1,2]")
  b = pyArray + [3, 4]
  assert pyArray == [1, 2]

# TODO
# def test_concat_array_right():
#    pyArray = pm.eval("[1,2]")
#    b = [3,4] + pyArray
#    assert b == [1,2,3,4]


def test_concat_pm_array_right():
  a = pm.eval("[1,2]")
  b = pm.eval("[3,4]")
  c = a + b
  assert c == [1, 2, 3, 4]

# repeat


def test_repeat_negative():
  a = pm.eval("[1,2]")
  b = a * -1
  assert b == []


def test_repeat_zero():
  a = pm.eval("[1,2]")
  b = a * 0
  assert b == []


def test_repeat_once():
  a = pm.eval("[1,2]")
  b = a * 1
  assert b == [1, 2]
  assert a is not b


def test_repeat_twice():
  a = pm.eval("[1,2]")
  b = a * 2
  assert b == [1, 2, 1, 2]


def test_repeat_wrong_type():
  a = pm.eval('([1,2])')
  try:
    a * 1.0
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "can't multiply sequence by non-int of type 'float'"


def test_repeat_not_in_place():
  a = pm.eval("[1,2]")
  b = a * 3
  assert b == [1.0, 2.0, 1.0, 2.0, 1.0, 2.0]
  assert a == [1, 2]

# contains


def test_contains_found():
  a = pm.eval("[1,2]")
  assert 1 in a


def test_contains_not_found():
  a = pm.eval("[1,2]")
  assert 7 not in a


def test_contains_not_found_odd_type():
  a = pm.eval("[1,2]")
  assert [1] not in a

# concat in place  +=


def test_concat_is_inplace():
  pyArray = pm.eval("[1,2]")
  pyArray += [3]
  assert pyArray == [1, 2, 3]


def test_concat_in_place_empty_empty():
  pyArray = pm.eval("[]")
  pyArray += []
  assert pyArray == []


def test_concat_not_empty_empty():
  pyArray = pm.eval("[1,2]")
  pyArray += []
  assert pyArray == [1, 2]

# repeat in place  *=


def test_repeat_is_in_place():
  a = pm.eval("[1,2]")
  a *= 3
  assert a == [1, 2, 1, 2, 1, 2]


def test_repeat_negative():
  a = pm.eval("[1,2]")
  a *= -1
  assert a == []


def test_repeat_zero():
  a = pm.eval("[1,2]")
  a *= 0
  assert a == []


def test_repeat_twice():
  a = pm.eval("[1,2]")
  a *= 2
  assert a == [1, 2, 1, 2]


def test_repeat_wrong_type():
  a = pm.eval('([1,2])')
  try:
    a *= 1.7
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "can't multiply sequence by non-int of type 'float'"


# clear
def test_clear():
  a = pm.eval("[1,2]")
  a.clear()
  assert a == []


def test_clear_with_arg():
  a = pm.eval('([1,2])')
  try:
    a.clear(8)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e).__contains__('clear() takes no arguments (1 given)')

# copy


def test_copy():
  a = pm.eval("[1,2]")
  b = a.copy()
  b[0] = 8
  assert a[0] == 1
  assert b[0] == 8


def test_copy_with_arg():
  a = pm.eval('([1,2])')
  try:
    b = a.copy(8)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e).__contains__('copy() takes no arguments (1 given)')

# append


def test_append():
  a = pm.eval("[1,2]")
  b = a.append(3)
  assert a == [1, 2, 3]
  assert b is None

# insert


def test_insert_list():
  a = pm.eval("[1,2]")
  a.insert(0, [3, 4])
  assert a == [[3, 4], 1, 2]


def test_insert_boolean():
  a = pm.eval("[1,2]")
  a.insert(0, True)
  assert a == [True, 1, 2]


def test_insert_int():
  a = pm.eval("[1,2]")
  a.insert(0, 3)
  assert a == [3, 1, 2]


def test_insert_float():
  a = pm.eval("[1,2]")
  a.insert(0, 4.5)
  assert a == [4.5, 1, 2]


def test_insert_string():
  a = pm.eval("[1,2]")
  a.insert(0, "Hey")
  assert a == ["Hey", 1, 2]


def test_insert_none():
  a = pm.eval("[1,2]")
  a.insert(0, None)
  assert a == [None, 1, 2]


def test_insert_function():
  def f():
    return
  a = pm.eval("[1,2]")
  a.insert(0, f)
  assert len(a) == 3


def test_insert_int_neg_index():
  a = pm.eval("[1,2]")
  a.insert(-1, 3)
  assert a == [1, 3, 2]


def test_insert_int_too_large_index():
  a = pm.eval("[1,2]")
  a.insert(5, 3)
  assert a == [1, 2, 3]


def test_insert_int_too_many_args():
  a = pm.eval('([1,2])')
  try:
    a.insert(5, 6, 7)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "insert expected 2 arguments, got 3"


def test_insert_int_too_few_args():
  a = pm.eval('([1,2])')
  try:
    a.insert()
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "insert expected 2 arguments, got 0"

# extend


def test_extend_with_list():
  a = pm.eval("[1,2]")
  a.extend([3, 4])
  assert a == [1, 2, 3, 4]


def test_extend_with_dict_single():
  a = pm.eval("[1,2]")
  a.extend({'key': 5})
  assert a == [1, 2, 'key']


def test_extend_with_dict_double():
  a = pm.eval("[1,2]")
  a.extend({'key': 5, 'otherKey': 6})
  assert a == [1, 2, 'key', 'otherKey']


def test_extend_with_number():
  a = pm.eval('([1,2])')
  try:
    a.extend(3)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "'int' object is not iterable"


def test_extend_with_own_list():
  a = pm.eval("[1,2]")
  a.extend(a)
  assert a == [1, 2, 1, 2]


def test_extend_with_pm_list():
  a = pm.eval("[1,2]")
  b = pm.eval("[3,4]")
  a.extend(b)
  assert a == [1, 2, 3, 4]

# TODO iterable dict


def test_extend_with_own_dict():
  a = pm.eval("[1,2]")
  b = pm.eval("({'key':5, 'key2':6})")
  a.extend(b)
  assert a == [1, 2, "key", "key2"]

# pop


def test_pop_no_arg():
  a = pm.eval("[1,2]")
  b = a.pop()
  assert a == [1]
  assert b == 2


def test_pop_empty_list():
  a = pm.eval('([])')
  try:
    a.pop()
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'IndexError'>"
    assert str(e) == "pop from empty list"


def test_pop_list_no_arg():
  a = pm.eval("[1,[2,3]]")
  b = a.pop()
  assert a == [1]
  assert b == [2, 3]


def test_pop_first_item():
  a = pm.eval("[1,2]")
  b = a.pop(0)
  assert a == [2]
  assert b == 1


def test_pop_second_item():
  a = pm.eval("[1,2]")
  b = a.pop(1)
  assert a == [1]
  assert b == 2


def test_pop_negative_item():
  a = pm.eval("[1,2]")
  b = a.pop(-1)
  assert a == [1]
  assert b == 2


def test_pop_out_of_bounds_item():
  a = pm.eval('([1,2])')
  try:
    a.pop(3)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'IndexError'>"
    assert str(e) == "pop index out of range"

# remove


def test_remove_found_once():
  a = pm.eval("[1,2]")
  a.remove(1)
  assert a == [2]


def test_remove_found_twice():
  a = pm.eval("[1,2,1,2]")
  a.remove(1)
  assert a == [2, 1, 2]


def test_remove_no_args():
  a = pm.eval('([1,2])')
  try:
    a.remove()
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e).__contains__('remove() takes exactly one argument (0 given)')


def test_remove_not_found():
  a = pm.eval('([1,2])')
  try:
    a.remove(3)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'ValueError'>"
    assert str(e) == "list.remove(x): x not in list"

# index


def test_index_found():
  a = pm.eval("[1,2,3,4]")
  b = a.index(3)
  assert b == 2


def test_index_not_found():
  a = pm.eval('([1,2])')
  try:
    a.index(3)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'ValueError'>"
    assert str(e) == "3 is not in list"


def test_index_no_args():
  a = pm.eval('([1,2,3,4])')
  try:
    a.index()
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "index expected at least 1 argument, got 0"


def test_index_too_many_args():
  a = pm.eval('([1,2,3,4])')
  try:
    a.index(2, 3, 4, 5)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "index expected at most 3 arguments, got 4"


def test_index_found_with_start():
  a = pm.eval("[1,2,3,4]")
  b = a.index(3, 0)
  assert b == 2


def test_index_found_with_negative_start():
  a = pm.eval("[1,2,3,4]")
  b = a.index(3, -3)
  assert b == 2


def test_index_not_found_with_start():
  a = pm.eval('([1,2,3,4])')
  try:
    a.index(3, 4)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'ValueError'>"
    assert str(e) == "3 is not in list"


def test_index_found_with_start_and_stop():
  a = pm.eval("[1,2,3,4]")
  b = a.index(3, 1, 4)
  assert b == 2


def test_index_found_with_start_and_negative_stop():
  a = pm.eval("[1,2,3,4]")
  b = a.index(3, 1, -1)
  assert b == 2


def test_index_not_found_with_start_and_stop():
  a = pm.eval('([1,2,3,4])')
  try:
    a.index(3, 4, 4)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'ValueError'>"
    assert str(e) == "3 is not in list"


def test_index_not_found_with_start_and_outofbounds_stop():
  a = pm.eval('([1,2,3,4])')
  try:
    a.index(3, 4, 7)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'ValueError'>"
    assert str(e) == "3 is not in list"

# count


def test_count_found_once():
  a = pm.eval("[1,2,3,4]")
  b = a.count(1)
  assert b == 1


def test_count_found_once_non_primitive_type():
  a = pm.eval("[1,2,[3,4]]")
  b = a.count([3, 4])
  assert b == 1


def test_count_found_twice():
  a = pm.eval("[1,2,3,4,5,1,2]")
  b = a.count(2)
  assert b == 2


def test_count_not_found():
  a = pm.eval("[1,2,3,4,5,1,2]")
  b = a.count(7)
  assert b == 0

# reverse


def test_reverse():
  a = pm.eval("[1,2,3,4]")
  b = a.reverse()
  assert a == [4, 3, 2, 1]
  assert b is None


def test_reverse_zero_length():
  a = pm.eval("[]")
  a.reverse()
  assert a == []


def test_reverse_one_length():
  a = pm.eval("[2]")
  a.reverse()
  assert a == [2]


def test_reverse_too_many_args():
  a = pm.eval('([1,2,3,4])')
  try:
    a.reverse(3)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e).__contains__('reverse() takes no arguments (1 given)')

# sort


def test_sort():
  a = pm.eval("[5,1,2,4,9,6,3,7,8]")
  a.sort()
  assert a == [1, 2, 3, 4, 5, 6, 7, 8, 9]


def test_sort_reverse():
  a = pm.eval("[5,1,2,4,9,6,3,7,8]")
  a.sort(reverse=True)
  assert a == [9, 8, 7, 6, 5, 4, 3, 2, 1]


def test_sort_reverse_false():
  a = pm.eval("[5,1,2,4,9,6,3,7,8]")
  a.sort(reverse=False)
  assert a == [1, 2, 3, 4, 5, 6, 7, 8, 9]


def test_sort_with_function():
  def myFunc(e, f):
    return len(e) - len(f)
  a = pm.eval("(['Ford', 'Mitsubishi', 'BMW', 'VW'])")
  a.sort(key=myFunc)
  assert a == ['VW', 'BMW', 'Ford', 'Mitsubishi']


def test_sort_with_js_function():
  a = pm.eval("(['Ford', 'Mitsubishi', 'BMW', 'VW'])")
  myFunc = pm.eval("((a, b) => a.toLocaleUpperCase() < b.toLocaleUpperCase() ? -1 : 1)")
  a.sort(key=myFunc)
  assert a == ['BMW', 'Ford', 'Mitsubishi', 'VW']


def test_sort_with_one_arg_function():
  def myFunc(e):
    return len(e)
  a = pm.eval("(['Ford', 'Mitsubishi', 'BMW', 'VW'])")
  a.sort(key=myFunc)
  assert a == ['VW', 'BMW', 'Ford', 'Mitsubishi']


def test_sort_with_one_arg_function_wrong_data_type():
  def myFunc(e):
    return len(e)
  a = pm.eval('([1,2,3,4])')
  try:
    a.sort(key=myFunc)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "object of type 'float' has no len()"


def test_sort_with_function_two_args_and_reverse_false():
  def myFunc(e, f):
    return len(e) - len(f)
  a = pm.eval("(['Ford', 'Mitsubishi', 'BMW', 'VW'])")
  a.sort(key=myFunc, reverse=False)
  assert a == ['VW', 'BMW', 'Ford', 'Mitsubishi']


def test_sort_with_js_function_and_reverse_false():
  a = pm.eval("(['Ford', 'Mitsubishi', 'BMW', 'VW'])")
  myFunc = pm.eval("((a, b) => a.toLocaleUpperCase() < b.toLocaleUpperCase() ? -1 : 1)")
  a.sort(key=myFunc, reverse=False)
  assert a == ['BMW', 'Ford', 'Mitsubishi', 'VW']


def test_sort_with_function_and_reverse():
  def myFunc(e, f):
    return len(e) - len(f)
  a = pm.eval("(['Ford', 'Mitsubishi', 'BMW', 'VW'])")
  a.sort(key=myFunc, reverse=True)
  assert a == ['Mitsubishi', 'Ford', 'BMW', 'VW']


def test_sort_with_js_function_and_reverse():
  a = pm.eval("(['Ford', 'Mitsubishi', 'BMW', 'VW'])")
  myFunc = pm.eval("((a, b) => a.toLocaleUpperCase() < b.toLocaleUpperCase() ? -1 : 1)")
  a.sort(key=myFunc, reverse=True)
  assert a == ['VW', 'Mitsubishi', 'Ford', 'BMW']


def test_sort_with_function_wrong_type():
  a = pm.eval('([1,2,3,4])')
  try:
    b = 9
    a.sort(key=b)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "'int' object is not callable"


def test_tricky_sort():
  a = pm.eval("[6, -2, 2, -7]")
  a.sort()
  assert a == [-7, -2, 2, 6]


def test_tricky_sort_reverse():
  a = pm.eval("[6, -2, 2, -7]")
  a.sort(reverse=True)
  assert a == [6, 2, -2, -7]


def test_sort_with_builtin_function():     # + wrong type of entries
  a = pm.eval("(['Ford', 'Mitsubishi', 'BMW', 'VW'])")
  a.sort(key=len)
  assert a == ['VW', 'BMW', 'Ford', 'Mitsubishi']


def test_sort_with_builtin_function_and_reverse():     # + wrong type of entries
  a = pm.eval("(['Ford', 'Mitsubishi', 'BMW', 'VW'])")
  a.sort(key=len, reverse=True)
  assert a == ['Mitsubishi', 'Ford', 'BMW', 'VW']


def test_sort_with_builtin_function_wrong_data_type():
  a = pm.eval('([1,2,3,4])')
  try:
    a.sort(key=len)
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "object of type 'float' has no len()"

# iter


def iter_min():
  a = pm.eval("([7,9,1,2,3,4,5,6])")
  b = min(a)
  assert b == 1


def iter_max():
  a = pm.eval("([7,9,1,2,3,4,5,6])")
  b = max(a)
  assert b == 9


def iter_for():
  a = pm.eval("(['this is a test', 'another test'])")
  b = [item.upper() for item in a]
  assert b == ['THIS IS A TEST', 'ANOTHER TEST']


def test_reduce():
  a = pm.eval("([1, 3, 5, 6, 2])")
  result = reduce(lambda a, b: a + b, a)
  assert result == 17


def test_iter_next():
  a = pm.eval("([1, 3, 5, 6, 2])")
  iterator = iter(a)
  try:
    while True:
      element = next(iterator)
    assert (False)
  except StopIteration:
    assert (True)

# reverse_iter


def iter_reverse():
  a = pm.eval("(['7','9','1','2','3','4','5','6'])")
  b = ""
  for i in reversed(a):
    b += i
  assert b == '65432197'

# slice subscript


def test_slice_full_array_single_subscript():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[:]
  assert b == [1, 2, 3, 4, 5, 6]


def test_slice_full_array_double_subscript():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[::]
  assert b == [1, 2, 3, 4, 5, 6]


def test_slice_empty_length_left():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[len(a):]
  assert b == []


def test_slice_full_length_right():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[:len(a)]
  assert b == [1, 2, 3, 4, 5, 6]


def test_slice_zero_to_length():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[0:6]
  assert b == [1, 2, 3, 4, 5, 6]


def test_slice():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[1:5]
  assert b == [2, 3, 4, 5]


def test_slice_negative_start():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[-3:5]
  assert b == [4, 5]


def test_slice_negative_end():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[2:-1]
  assert b == [3, 4, 5]


def test_slice_negative_start_negative_end():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[-3:-1]
  assert b == [4, 5]


def test_slice_step_zero():
  a = pm.eval('([1,2,3,4])')
  try:
    a[0:6:0]
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'ValueError'>"
    assert str(e) == "slice step cannot be zero"


def test_slice_step_negative():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[0:5:-1]
  assert b == []


def test_slice_step_one():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[1:5:1]
  assert b == [2, 3, 4, 5]


def test_slice_step_two():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[1:5:2]
  assert b == [2, 4]


def test_slice_step_three():
  a = pm.eval("([1,2,3,4,5,6])")
  b = a[1:5:3]
  assert b == [2, 5]

# slice subscript assign


def test_slice_assign_partial_array():
  a = pm.eval("([1,2,3,4,5,6])")
  a[2:4] = [7, 8, 9, 0, 1, 2]
  assert a == [1, 2, 7, 8, 9, 0, 1, 2, 5, 6]


def test_slice_delete_partial_array():
  a = pm.eval("([1,2,3,4,5,6])")
  del a[2:4]
  assert a == [1, 2, 5, 6]


def test_slice_assign_own_array():
  a = pm.eval("([1,2,3,4,5,6])")
  a[2:4] = a
  assert a == [1, 2, 1, 2, 3, 4, 5, 6, 5, 6]


def test_slice_assign_pm_array():
  a = pm.eval("([1,2,3,4,5,6])")
  b = pm.eval("([7,8])")
  a[2:4] = b
  assert a == [1, 2, 7, 8, 5, 6]


def test_slice_assign_wrong_type():
  a = pm.eval('([1,2,3,4])')
  try:
    a[2:4] = 6
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "can only assign an iterable"


def test_slice_assign_negative_low():
  a = pm.eval("([1,2,3,4,5,6])")
  a[-3:6] = [7, 8]
  assert a == [1, 2, 3, 7, 8]


def test_slice_assign_negative_low_negative_high():
  a = pm.eval("([1,2,3,4,5,6])")
  a[-3:-1] = [7, 8]
  assert a == [1, 2, 3, 7, 8, 6]


def test_slice_assign_high_larger_than_length():
  a = pm.eval("([1,2,3,4,5,6])")
  a[1:8] = [7, 8]
  assert a == [1, 7, 8]


def test_slice_assign_clear():
  a = pm.eval("([1,2,3,4,5,6,7,8,9,1,2])")
  a[0:32] = []
  assert a == []


def test_slice_delete_partial_array_step_negative():
  a = pm.eval("([1,2,3,4,5,6])")
  del a[0:4:-1]
  assert a == [1, 2, 3, 4, 5, 6]


def test_slice_delete_partial_array_step_one():
  a = pm.eval("([1,2,3,4,5,6])")
  del a[2:4:1]
  assert a == [1, 2, 5, 6]


def test_slice_delete_partial_array_step_two():
  a = pm.eval("([1,2,3,4,5,6])")
  del a[2:6:2]
  assert a == [1, 2, 4, 6]


def test_slice_delete_partial_array_step_three():
  a = pm.eval("([1,2,3,4,5,6])")
  del a[0:6:3]
  assert a == [2, 3, 5, 6]


def test_slice_delete_partial_array_step_two_negative_start():
  a = pm.eval("([1,2,3,4,5,6])")
  del a[-5:6:2]
  assert a == [1, 3, 5]


def test_slice_delete_partial_array_step_two_negative_start_negative_end():
  a = pm.eval("([1,2,3,4,5,6])")
  del a[-5:-2:2]
  assert a == [1, 3, 5, 6]


def test_slice_assign_step_wrong_list_size():
  a = pm.eval('([1,2,3,4])')
  try:
    a[0:4:2] = [1]
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'ValueError'>"
    assert str(e) == "attempt to assign sequence of size 1 to extended slice of size 2"


def test_slice_assign_partial_array_step_negative():
  a = pm.eval("([1,2,3,4,5,6])")
  a[2:4:2] = [7]
  assert a == [1, 2, 7, 4, 5, 6]


def test_slice_assign_step_zero():
  a = pm.eval('([1,2,3,4])')
  try:
    a[0:4:0] = [1]
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'ValueError'>"
    assert str(e) == "slice step cannot be zero"


def test_slice_assign_partial_array_step_2():
  a = pm.eval("([1,2,3,4,5,6])")
  a[2:4:2] = [7]
  assert a == [1, 2, 7, 4, 5, 6]


def test_slice_assign_step_wrong_type():
  a = pm.eval('([1,2,3,4])')
  try:
    a[2:4:2] = 6
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'TypeError'>"
    assert str(e) == "must assign iterable to extended slice"


def test_slice_assign_partial_array_negative_start():
  a = pm.eval("([1,2,3,4,5,6])")
  a[-5:4:2] = [7, 8]
  assert a == [1, 7, 3, 8, 5, 6]


def test_slice_assign_partial_array_negative_start_negative_stop():
  a = pm.eval("([1,2,3,4,5,6])")
  a[-5:-1:2] = [7, 8]
  assert a == [1, 7, 3, 8, 5, 6]


def test_slice_assign_own_array_no_match():
  a = pm.eval("([1,2,3,4,5,6])")
  try:
    a[0:4:2] = a
    assert (False)
  except Exception as e:
    assert str(type(e)) == "<class 'ValueError'>"
    assert str(e) == "attempt to assign sequence of size 0 to extended slice of size 2"


def test_slice_assign_pm_array_step_2():
  a = pm.eval("([1,2,3,4,5,6])")
  b = pm.eval("([1,2,3])")
  a[0:10:2] = b
  assert a == [1, 2, 2, 4, 3, 6]

# __class__


def test___class__attribute():
  items = pm.eval("([1,2,3,4,5,6])")
  assert repr(items.__class__) == "<class 'list'>"
