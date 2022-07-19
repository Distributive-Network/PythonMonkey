add_test([=[HelloTest.BasicAssertions]=]  /Users/johntedesco/KDS/new_py2cpp/build/tests/hello_test [==[--gtest_filter=HelloTest.BasicAssertions]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HelloTest.BasicAssertions]=]  PROPERTIES WORKING_DIRECTORY /Users/johntedesco/KDS/new_py2cpp/build/tests SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  hello_test_TESTS HelloTest.BasicAssertions)
