cd build
cmake ..
cmake --build .
cd tests
ctest --rerun-failed --output-on-failure
cd ../../tests/python
ln -sf ../../python/pythonmonkey .
python -m pytest .
