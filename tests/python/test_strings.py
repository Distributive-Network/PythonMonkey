import pythonmonkey as pm
import gc
import random
import copy


def test_identity():
  py_string = "abc"
  js_string = pm.eval("(str) => str")(py_string)
  assert py_string is js_string


def test_eval_ascii_string_matches_evaluated_string():
  py_ascii_string = "abc"
  js_ascii_string = pm.eval(repr(py_ascii_string))
  assert py_ascii_string == js_ascii_string


def test_eval_latin1_string_matches_evaluated_string():
  py_latin1_string = "a©Ð"
  js_latin1_string = pm.eval(repr(py_latin1_string))
  assert py_latin1_string == js_latin1_string


def test_eval_null_character_string_matches_evaluated_string():
  py_null_character_string = "a\x00©"
  js_null_character_string = pm.eval(repr(py_null_character_string))
  assert py_null_character_string == js_null_character_string


def test_eval_ucs2_string_matches_evaluated_string():
  py_ucs2_string = "ՄԸՋ"
  js_ucs2_string = pm.eval(repr(py_ucs2_string))
  assert py_ucs2_string == js_ucs2_string


def test_eval_unpaired_surrogate_string_matches_evaluated_string():
  py_unpaired_surrogate_string = "Ջ©\ud8fe"
  js_unpaired_surrogate_string = pm.eval(repr(py_unpaired_surrogate_string))
  assert py_unpaired_surrogate_string == js_unpaired_surrogate_string


def test_eval_ucs4_string_matches_evaluated_string():
  py_ucs4_string = "🀄🀛🜢"
  js_ucs4_string = pm.eval(repr(py_ucs4_string))
  assert py_ucs4_string == js_ucs4_string


def test_eval_latin1_string_fuzztest():
  n = 10
  for _ in range(n):
    length = random.randint(0x0000, 0xFFFF)
    string1 = ''

    for _ in range(length):
      codepoint = random.randint(0x00, 0xFF)
      string1 += chr(codepoint)  # add random chr in latin1 range

    INITIAL_STRING = string1
    m = 10
    for _ in range(m):
      string2 = pm.eval(repr(string1))
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      gc.collect()
      pm.collect()

      # garbage collection should not collect variables still in scope
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      string1 = string2
    assert INITIAL_STRING == string1  # strings should still match after a bunch of iterations through JS


def test_eval_ucs2_string_fuzztest():
  n = 10
  for _ in range(n):
    length = random.randint(0x0000, 0xFFFF)
    string1 = ''

    for _i in range(length):
      codepoint = random.randint(0x00, 0xFFFF)
      string1 += chr(codepoint)  # add random chr in ucs2 range

    INITIAL_STRING = string1
    m = 10
    for _ in range(m):
      string2 = pm.eval(repr(string1))
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      gc.collect()
      pm.collect()

      # garbage collection should not collect variables still in scope
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      string1 = string2
    assert INITIAL_STRING == string1  # strings should still match after a bunch of iterations through JS


def test_eval_ucs4_string_fuzztest():
  n = 10
  for _ in range(n):
    length = random.randint(0x0000, 0xFFFF)
    string1 = ''

    for _ in range(length):
      codepoint = random.randint(0x010000, 0x10FFFF)
      string1 += chr(codepoint)  # add random chr outside BMP

    INITIAL_STRING = string1
    m = 10
    for _ in range(m):
      string2 = pm.eval("'" + string1 + "'")
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      gc.collect()
      pm.collect()

      # garbage collection should not collect variables still in scope
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      string1 = string2
    assert INITIAL_STRING == string1  # strings should still match after a bunch of iterations through JS


def test_eval_boxed_ascii_string_matches_evaluated_string():
  py_ascii_string = "abc"
  js_ascii_string = pm.eval(f'new String({repr(py_ascii_string)})')
  assert py_ascii_string == js_ascii_string


def test_eval_boxed_latin1_string_matches_evaluated_string():
  py_latin1_string = "a©Ð"
  js_latin1_string = pm.eval(f'new String({repr(py_latin1_string)})')
  assert py_latin1_string == js_latin1_string


def test_eval_boxed_null_character_string_matches_evaluated_string():
  py_null_character_string = "a\x00©"
  js_null_character_string = pm.eval(f'new String({repr(py_null_character_string)})')
  assert py_null_character_string == js_null_character_string


def test_eval_boxed_ucs2_string_matches_evaluated_string():
  py_ucs2_string = "ՄԸՋ"
  js_ucs2_string = pm.eval(f'new String({repr(py_ucs2_string)})')
  assert py_ucs2_string == js_ucs2_string


def test_eval_boxed_unpaired_surrogate_string_matches_evaluated_string():
  py_unpaired_surrogate_string = "Ջ©\ud8fe"
  js_unpaired_surrogate_string = pm.eval(f'new String({repr(py_unpaired_surrogate_string)})')
  assert py_unpaired_surrogate_string == js_unpaired_surrogate_string


def test_eval_boxed_ucs4_string_matches_evaluated_string():
  py_ucs4_string = "🀄🀛🜢"
  js_ucs4_string = pm.eval(f'new String({repr(py_ucs4_string)})')
  assert py_ucs4_string == js_ucs4_string


def test_eval_boxed_latin1_string_fuzztest():
  n = 10
  for _ in range(n):
    length = random.randint(0x0000, 0xFFFF)
    string1 = ''

    for _ in range(length):
      codepoint = random.randint(0x00, 0xFF)
      string1 += chr(codepoint)  # add random chr in latin1 range

    INITIAL_STRING = string1
    m = 10
    for _ in range(m):
      string2 = pm.eval(f'new String({repr(string1)})')
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      gc.collect()
      pm.collect()

      # garbage collection should not collect variables still in scope
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      string1 = string2
    assert INITIAL_STRING == string1  # strings should still match after a bunch of iterations through JS


def test_eval_boxed_ucs2_string_fuzztest():
  n = 10
  for _ in range(n):
    length = random.randint(0x0000, 0xFFFF)
    string1 = ''

    for _ in range(length):
      codepoint = random.randint(0x00, 0xFFFF)
      string1 += chr(codepoint)  # add random chr in ucs2 range

    INITIAL_STRING = string1
    m = 10
    for _ in range(m):
      string2 = pm.eval(f'new String({repr(string1)})')
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      gc.collect()
      pm.collect()

      # garbage collection should not collect variables still in scope
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      string1 = string2
    assert INITIAL_STRING == string1  # strings should still match after a bunch of iterations through JS


def test_eval_boxed_ucs4_string_fuzztest():
  n = 10
  for _ in range(n):
    length = random.randint(0x0000, 0xFFFF)
    string1 = ''

    for _ in range(length):
      codepoint = random.randint(0x010000, 0x10FFFF)
      string1 += chr(codepoint)  # add random chr outside BMP

    INITIAL_STRING = string1
    m = 10
    for _ in range(m):
      string2 = pm.eval(f'new String("{string1}")')
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      gc.collect()
      pm.collect()

      # garbage collection should not collect variables still in scope
      assert len(string1) == length
      assert len(string2) == length
      assert len(string1) == len(string2)
      assert string1 == string2

      string1 = string2
    assert INITIAL_STRING == string1  # strings should still match after a bunch of iterations through JS


def test_string_proxy_copy():
  world = pm.eval('(function() {return "World"})')
  say = pm.eval('(function(who) { return `Hello ${who}`})')
  who = world()
  hello_world = say(copy.copy(who))
  assert hello_world == "Hello World"
  assert hello_world is not who

def test_string_proxy_deepcopy():
  world = pm.eval('(function() {return "World"})')
  say = pm.eval('(function(who) { return `Hello ${who}`})')
  who = world()
  hello_world = say(copy.deepcopy(who))
  assert hello_world == "Hello World"
  assert hello_world is not who
  