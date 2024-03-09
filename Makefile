# @file		Makefile
#               Not part of the PythonMonkey build - just workflow helper for Wes.
# @author       Wes Garland, wes@distributive.network
# @date         March 2024
#

BUILD  = debug
PYTHON = python3
RUN    = poetry run

EXTRA_CMAKE_CXX_FLAGS = -Wno-invalid-offsetof

ifeq ($(BUILD),debug)
EXTRA_CMAKE_CXX_FLAGS += -O0
endif

.PHONY: build test all clean
build:
	VERBOSE=1 EXTRA_CMAKE_CXX_FLAGS="$(EXTRA_CMAKE_CXX_FLAGS)" $(PYTHON) ./build.py

test:
	$(RUN) ./peter-jr tests
	$(RUN) pytest tests/python

all:	build test

clean:
	rm -rf build/src/CMakeFiles/pythonmonkey.dir
	rm -f build/src/pythonmonkey.so
	rm -f python/pythonmonkey.so
