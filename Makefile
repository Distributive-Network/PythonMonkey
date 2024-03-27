# @file		Makefile
#               Not part of the PythonMonkey build - just workflow helper for Wes.
# @author       Wes Garland, wes@distributive.network
# @date         March 2024
#

BUILD  = debug
PYTHON = python3
RUN    = poetry run

PYTHON_BUILD_ENV = VERBOSE=1 EXTRA_CMAKE_CXX_FLAGS="$(EXTRA_CMAKE_CXX_FLAGS)"
OS_NAME := $(shell uname -s)

ifeq ($(OS_NAME),Linux)
CPU_COUNT=$(shell cat /proc/cpuinfo  | grep -c processor)
MAX_JOBS=10
CPUS := $(shell test $(CPU_COUNT) -lt $(MAX_JOBS) && echo $(CPU_COUNT) || echo $(MAX_JOBS))
PYTHON_BUILD_ENV += CPUS=$(CPUS)
endif

EXTRA_CMAKE_CXX_FLAGS = -Wno-invalid-offsetof $(JOBS)

ifeq ($(BUILD),debug)
EXTRA_CMAKE_CXX_FLAGS += -O0
endif

.PHONY: build test all clean debug
build:
	$(PYTHON_BUILD_ENV) $(PYTHON) ./build.py

test:
	$(RUN) ./peter-jr tests
	$(RUN) pytest tests/python

all:	build test

clean:
	rm -rf build/src/CMakeFiles/pythonmonkey.dir
	rm -f build/src/pythonmonkey.so
	rm -f python/pythonmonkey.so

debug:
	@echo EXTRA_CMAKE_CXX_FLAGS=$(EXTRA_CMAKE_CXX_FLAGS)
	@echo JOBS=$(JOBS)
	@echo CPU_COUNT=$(CPU_COUNT)
	@echo OS_NAME=$(OS_NAME)
