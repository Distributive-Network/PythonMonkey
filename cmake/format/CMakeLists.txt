# Copyright (c) 2022 Distributive Inc. All Rights Reserved.

if(NOT UNCRUSTIFY_EXECUTABLE)
  if(UNCRUSTIFY_ROOT STREQUAL "")
    message(STATUS "Target '${PROJECT_NAME}-format-cpp' not added:"
      " UNCRUSTIFY_ROOT not set"
    )
    return()
  endif()

  find_program(UNCRUSTIFY_EXECUTABLE
    NAMES "uncrustify"
    DOC "Uncrustify executable path"
    PATHS "${UNCRUSTIFY_ROOT}"
    PATH_SUFFIXES "bin"
    NO_DEFAULT_PATH
  )
  if(NOT UNCRUSTIFY_EXECUTABLE)
    message(STATUS "Target '${PROJECT_NAME}-format-cpp' not added:"
      " Uncrustify not found"
    )
    return()
  endif()
endif()
message(STATUS "Using Uncrustify: ${UNCRUSTIFY_EXECUTABLE}")

option(PYTHONMONKEY_CPP_FORMAT_FIX "Automatically fix formatting errors." ON)
if(PYTHONMONKEY_CPP_FORMAT_FIX)
  message(STATUS "Automatically fixing C++ formatting errors")
  set(PYTHONMONKEY_CPP_FORMAT_OPTIONS "--replace" "--if-changed" "--no-backup")
  set(PYTHONMONKEY_CPP_FORMAT_COMMENT "Checking and fixing code formatting...")
else()
  message(STATUS "Reporting C++ formatting errors without fixing")
  set(PYTHONMONKEY_CPP_FORMAT_OPTIONS "--check")
  string(CONCAT PYTHONMONKEY_CPP_FORMAT_COMMENT "Checking code formatting"
    " (regenerate with PYTHONMONKEY_CPP_FORMAT_FIX=ON to automatically fix errors)..."
  )
endif()

file(GLOB_RECURSE PYTHONMONKEY_CPP_FORMAT_FILES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/include/*.hh"
  "${PROJECT_SOURCE_DIR}/src/*.cc"
)

set(PYTHONMONKEY_CPP_FORMAT_COMMAND
  "${UNCRUSTIFY_EXECUTABLE}" "-c" "uncrustify.cfg"
  ${PYTHONMONKEY_CPP_FORMAT_OPTIONS}
  ${PYTHONMONKEY_CPP_FORMAT_FILES}
)

add_custom_target("${PROJECT_NAME}-format-cpp" ALL
  COMMAND ${PYTHONMONKEY_CPP_FORMAT_COMMAND}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "${PYTHONMONKEY_CPP_FORMAT_COMMENT}"
  VERBATIM
  SOURCES "uncrustify.cfg"
)
# pythonmonkey_target_initialize("${PROJECT_NAME}-format-cpp")

if(NOT AUTOPEP8_SCRIPT)
  if(AUTOPEP8_ROOT STREQUAL "")
    message(STATUS "Target '${PROJECT_NAME}-format-python' not added:"
      " AUTOPEP8_ROOT not set"
    )
    return()
  endif()

  find_program(AUTOPEP8_SCRIPT
    NAMES "autopep8.py"
    DOC "autopep8 script path"
    PATHS "${AUTOPEP8_ROOT}"
    NO_DEFAULT_PATH
  )
  if(NOT AUTOPEP8_SCRIPT)
    message(STATUS "Target '${PROJECT_NAME}-format-python' not added:"
      " autopep8 not found"
    )
    return()
  endif()
endif()
message(STATUS "Using autopep8: ${AUTOPEP8_SCRIPT}")

option(PYTHONMONKEY_PYTHON_FORMAT_FIX "Automatically fix formatting errors." ON)
if(PYTHONMONKEY_PYTHON_FORMAT_FIX)
  message(STATUS "Automatically fixing Python formatting errors")
  set(PYTHONMONKEY_PYTHON_FORMAT_OPTIONS "--in-place" "--verbose" "--aggressive" "--aggressive")
  set(PYTHONMONKEY_PYTHON_FORMAT_COMMENT "Checking and fixing code formatting...")
else()
  message(STATUS "Reporting Python formatting errors without fixing")
  set(PYTHONMONKEY_PYTHON_FORMAT_OPTIONS "--diff" "--verbose" "--aggressive" "--aggressive")
  string(CONCAT PYTHONMONKEY_PYTHON_FORMAT_COMMENT "Checking code formatting"
    " (regenerate with PYTHONMONKEY_PYTHON_FORMAT_FIX=ON to automatically fix errors)..."
  )
endif()

file(GLOB_RECURSE PYTHONMONKEY_PYTHON_FORMAT_FILES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/python/*.py"
  "${PROJECT_SOURCE_DIR}/tests/python/*.py"
)

set(PYTHONMONKEY_PYTHON_FORMAT_COMMAND
  "python3 ${AUTOPEP8_SCRIPT}"
  ${PYTHONMONKEY_PYTHON_FORMAT_OPTIONS}
  ${PYTHONMONKEY_PYTHON_FORMAT_FILES}
)

execute_process(
  COMMAND ${PYTHONMONKEY_PYTHON_FORMAT_COMMAND}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)