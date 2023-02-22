
cd build/tests
ctest --rerun-failed --output-on-failure
cd ../src
python -m pytest ../../tests/python