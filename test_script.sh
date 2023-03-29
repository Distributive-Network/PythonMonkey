cd build
cmake ..
cmake --build .
cd tests
ctest --rerun-failed --output-on-failure
cd ../src
python -m pytest ../../tests/python