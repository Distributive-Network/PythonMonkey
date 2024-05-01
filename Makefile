# @file		Makefile
#               Not part of the PythonMonkey build - just workflow helper for Wes.
# @author       Wes Garland, wes@distributive.network
# @date         March 2024
#

BUILD = Debug	# (case-insensitive) Release, DRelease, Debug, Profile, or None
DOCS = false
VERBOSE = true
PYTHON = python3
RUN = poetry run

OS_NAME := $(shell uname -s)

ifeq ($(OS_NAME),Linux)
CPU_COUNT = $(shell cat /proc/cpuinfo  | grep -c processor)
MAX_JOBS = 10
CPUS := $(shell test $(CPU_COUNT) -lt $(MAX_JOBS) && echo $(CPU_COUNT) || echo $(MAX_JOBS))
PYTHON_BUILD_ENV += CPUS=$(CPUS)
endif

ifeq ($(BUILD),Profile)
PYTHON_BUILD_ENV += BUILD_TYPE=Profile
else ifeq ($(BUILD),Debug)
PYTHON_BUILD_ENV += BUILD_TYPE=Debug
else ifeq ($(BUILD),DRelease)
PYTHON_BUILD_ENV += BUILD_TYPE=DRelease
else ifeq($(BUILD), None)
PYTHON_BUILD_ENV += BUILD_TYPE=None
else # Release build
PYTHON_BUILD_ENV += BUILD_TYPE=Release
endif

ifeq ($(DOCS),true)
PYTHON_BUILD_ENV += BUILD_DOCS=1
endif

ifeq ($(VERBOSE),true)
PYTHON_BUILD_ENV += VERBOSE=1
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
	rm -f python/pythonmonkey/pythonmonkey.so

debug:
	@echo JOBS=$(JOBS)
	@echo CPU_COUNT=$(CPU_COUNT)
	@echo OS_NAME=$(OS_NAME)