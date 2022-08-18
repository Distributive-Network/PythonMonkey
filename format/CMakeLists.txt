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

option(BIFROST_CPP_FORMAT_FIX "Automatically fix formatting errors." ON)
if(BIFROST_CPP_FORMAT_FIX)
  message(STATUS "Automatically fixing C++ formatting errors")
  set(BIFROST_CPP_FORMAT_OPTIONS "--replace" "--if-changed" "--no-backup")
  set(BIFROST_CPP_FORMAT_COMMENT "Checking and fixing code formatting...")
else()
  message(STATUS "Reporting C++ formatting errors without fixing")
  set(BIFROST_CPP_FORMAT_OPTIONS "--check")
  string(CONCAT BIFROST_CPP_FORMAT_COMMENT "Checking code formatting"
    " (regenerate with BIFROST_CPP_FORMAT_FIX=ON to automatically fix errors)..."
  )
endif()

file(GLOB_RECURSE BIFROST_CPP_FORMAT_FILES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/include/*.hh"
  "${PROJECT_SOURCE_DIR}/src/*.cc"
  "${PROJECT_SOURCE_DIR}/tests/cpp/*.cc"
)

set(BIFROST_CPP_FORMAT_COMMAND
  "${UNCRUSTIFY_EXECUTABLE}" "-c" "uncrustify.cfg"
  ${BIFROST_CPP_FORMAT_OPTIONS}
  ${BIFROST_CPP_FORMAT_FILES}
)

add_custom_target("${PROJECT_NAME}-format-cpp" ALL
  COMMAND ${BIFROST_CPP_FORMAT_COMMAND}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "${BIFROST_CPP_FORMAT_COMMENT}"
  VERBATIM
  SOURCES "uncrustify.cfg"
)
# bifrost_target_initialize("${PROJECT_NAME}-format-cpp")

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

option(BIFROST_PYTHON_FORMAT_FIX "Automatically fix formatting errors." ON)
if(BIFROST_PYTHON_FORMAT_FIX)
  message(STATUS "Automatically fixing Python formatting errors")
  set(BIFROST_PYTHON_FORMAT_OPTIONS "--in-place" "--verbose" "--aggressive" "--aggressive")
  set(BIFROST_PYTHON_FORMAT_COMMENT "Checking and fixing code formatting...")
else()
  message(STATUS "Reporting Python formatting errors without fixing")
  set(BIFROST_PYTHON_FORMAT_OPTIONS "--diff" "--verbose" "--aggressive" "--aggressive")
  string(CONCAT BIFROST_PYTHON_FORMAT_COMMENT "Checking code formatting"
    " (regenerate with BIFROST_PYTHON_FORMAT_FIX=ON to automatically fix errors)..."
  )
endif()

file(GLOB_RECURSE BIFROST_PYTHON_FORMAT_FILES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/python/*.py"
  "${PROJECT_SOURCE_DIR}/tests/python/*.py"
)

set(BIFROST_PYTHON_FORMAT_COMMAND
  "python3 ${AUTOPEP8_SCRIPT}"
  ${BIFROST_PYTHON_FORMAT_OPTIONS}
  ${BIFROST_PYTHON_FORMAT_FILES}
)

execute_process(
  COMMAND ${BIFROST_PYTHON_FORMAT_COMMAND}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)