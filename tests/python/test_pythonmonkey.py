import pythonmonkey

def test_passes():
    assert True

def test_eval_ascii_string_matches_evaluated_string():
    ascii_string = pythonmonkey.eval("'abc'")
    assert ascii_string == 'abc'

def test_eval_latin1_string_matches_evaluated_string():
    latin1_string = pythonmonkey.eval("'a©Ð'")
    assert latin1_string == 'a©Ð'

def test_eval_null_character_string_matches_evaluated_string():
    null_character_string = pythonmonkey.eval("'a\\x00©'")
    assert null_character_string = 'a\x00©'

def test_eval_ucs2_string_matches_evaluated_string():
    ucs2_string = pythonmonkey.eval("'ՄԸՋ'")
    assert ucs2_string == 'ՄԸՋ'

def test_eval_unpaired_surrogate_string_matches_evaluated_string():
    unpaired_surrogate_string = pythonmonkey.eval("'Ջ©\\ud8fe'")
    assert unpaired_surrogate_string = 'Ջ©\ud8fe'

# TODO (Caleb Aikens) write test for ucs4, and a fuzz test