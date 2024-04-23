# Copyright (c) 2022 Distributive Inc. All Rights Reserved.

cmake_minimum_required(VERSION 3.25) # Set minimum cmake version

set(PYTHONMONKEY_VERSION "0") # to be automatically set by poetry-dynamic-versioning
string(REGEX MATCH ^[0-9]+\.[0-9]+\.[0-9]+ PYTHONMONKEY_VERSION ${PYTHONMONKEY_VERSION}) # the version number can only contain integers

project("PythonMonkey"
  VERSION ${PYTHONMONKEY_VERSION}
  DESCRIPTION "A tool for Javascript-Python interoperability."
  LANGUAGES "CXX"
)

# Set C++ settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add an external; appends to `PYTHONMONKEY_EXTERNAL_FILES` in the parent scope.
function(pythonmonkey_add_external PYTHONMONKEY_EXTERNAL)
  add_subdirectory("cmake/externals/${PYTHONMONKEY_EXTERNAL}")
  set(PYTHONMONKEY_EXTERNAL_FILE "cmake/externals/${PYTHONMONKEY_EXTERNAL}/CMakeLists.txt")
  source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/externals/${PYTHONMONKEY_EXTERNAL}"
    PREFIX "Externals\\${PYTHONMONKEY_EXTERNAL}"
    FILES "${PYTHONMONKEY_EXTERNAL_FILE}"
  )
  list(APPEND PYTHONMONKEY_EXTERNAL_FILES "${PYTHONMONKEY_EXTERNAL_FILE}")

  set(PYTHONMONKEY_EXTERNAL_FILES ${PYTHONMONKEY_EXTERNAL_FILES} PARENT_SCOPE)
endfunction()

file (GLOB SOURCE_FILES "src/*.cc" "src/internalBinding/*.cc") # Find all C++ files in the src directory
file (GLOB HEADER_FILES "include/*.hh") # Find all header files in the include directory
file (GLOB PYTHON_FILES "python/*.cc" "python/*.hh") # Find all the python bindings in the python directory

include_directories(${CMAKE_CURRENT_LIST_DIR})


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

  ### Code block from: https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html
  include(FetchContent)

  SET(COMPILE_FLAGS "-ggdb -Ofast -fno-rtti") # optimize but also emit debug symbols
  SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS} $ENV{EXTRA_CMAKE_CXX_FLAGS}")

  set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
  if(APPLE)
    find_package(Python 3.8 COMPONENTS Interpreter Development REQUIRED)
    find_package(SpiderMonkey REQUIRED)
    set(PYTHON_MAJOR $ENV{Python_VERSION_MAJOR})
    set(PYTHON_MINOR $ENV{Python_VERSION_MINOR})
    set(PYTHONLIBS_VERSION_STRING ${Python_VERSION})
    set(PYTHON_INCLUDE_DIR ${Python_INCLUDE_DIRS})
    set(PYTHON_LIBRARIES ${Python_LIBRARIES})
    message("Apple - Using Python:${Python_VERSION_MAJOR} - Libraries:${PYTHON_LIBRARIES} - IncludeDirs: ${PYTHON_INCLUDE_DIR}")
  elseif(UNIX)
    find_package(Python 3.8 COMPONENTS Interpreter Development REQUIRED)
    set(Python_FIND_VIRTUALENV FIRST) # (require cmake >= v3.15 and this is the default) use the Python version configured by pyenv if available
    set(PYTHON_LIBRARIES ${Python_LIBRARIES})
    set(PYTHON_INCLUDE_DIR ${Python_INCLUDE_DIRS})
    message("Linux - Using Python:${Python_VERSION_MAJOR}.${Python_VERSION_MINOR} - Libraries:${PYTHON_LIBRARIES} - IncludeDirs: ${PYTHON_INCLUDE_DIR}")
    find_package(SpiderMonkey REQUIRED)
  elseif(WIN32)
    find_package(PythonInterp 3.8 REQUIRED)
    find_package(PythonLibs 3.8   REQUIRED)
    find_package(SpiderMonkey REQUIRED)
    set(PYTHONLIBS_VERSION_STRING $ENV{PY_VERSION})
  endif()
  include_directories(${PYTHON_INCLUDE_DIRS})
  include_directories(${SPIDERMONKEY_INCLUDE_DIR})

  # Add doxygen if this is the main app
  find_package(Doxygen)
  if(Doxygen_FOUND)
      add_subdirectory(cmake/docs)
  else()
      message(STATUS "Doxygen not found. Not building docs.")
  endif()

endif()  

# Add compiled folder directories
add_subdirectory(src)
