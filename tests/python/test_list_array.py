import pythonmonkey as pm


def test_eval_array_is_list():
  pythonList = pm.eval('[]')
  assert isinstance(pythonList, list)

# extra nice but not necessary


def test_eval_array_is_list_type_string():
  pythonListTypeString = str(type(pm.eval('[]')))
  assert pythonListTypeString == "<class 'list'>"


def test_eval_list_is_array():
  items = [1, 2, 3]
  isArray = pm.eval('Array.isArray')(items)
  assert isArray


def test_typeof_array():
  items = [1, 2, 3]
  result = [None]
  pm.eval("(result, arr) => {result[0] = typeof arr}")(result, items)
  assert result[0] == 'object'


def test_instanceof_array():
  items = [1, 2, 3]
  result = [None]
  pm.eval("(result, arr) => {result[0] = arr instanceof Array}")(result, items)
  assert result[0]


def test_instanceof_object():
  items = [1, 2, 3]
  result = [None]
  pm.eval("(result, arr) => {result[0] = arr instanceof Object}")(result, items)
  assert result[0]


def test_not_instanceof_string():
  items = [1, 2, 3]
  result = [None]
  pm.eval("(result, arr) => {result[0] = arr instanceof String}")(result, items)
  assert not result[0]
