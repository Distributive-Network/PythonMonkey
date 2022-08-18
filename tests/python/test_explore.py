import explore


def test_passes():
    assert True


def test_output_outputs_correct_integers(capfd):
    explore.output(1, 65, 72)

    out, err = capfd.readouterr()
    assert out == "16572"


def test_output_outputs_correct_strings(capfd):

    explore.output("abcd", "newline\n", "tabs\t")

    out, err = capfd.readouterr()
    assert out == "abcdnewline\ntabs\t"


def test_output_outputs_correct_mixture(capfd):

    explore.output("abcd", 13, "some string\n", 65)

    out, err = capfd.readouterr()

    assert out == "abcd13some string\n65"
