cd build
cmake ..
cmake --build .
cd tests
ctest --rerun-failed --output-on-failure
cd ../../python/pythonmonkey
ln -sf ../../build/src/pythonmonkey.so .
cd ../../tests/python
ln -sf ../../python/pythonmonkey .
python -m pytest .
