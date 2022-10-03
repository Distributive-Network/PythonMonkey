# Copyright (c) 2022 Distributive Inc. All Rights Reserved.

cmake_minimum_required(VERSION 3.4...3.18) # Set minimum cmake version

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" BIFROST_VERSION)
string(STRIP "${BIFROST_VERSION}" BIFROST_VERSION)
project("Bifrost"
  VERSION "${BIFROST_VERSION}"
  DESCRIPTION "A tool for Javascript-Python interoperability."
  LANGUAGES "CXX"
)

# Set C++ settings.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQURIED ON)
# Add an external; appends to `BIFROST_EXTERNAL_FILES` in the parent scope.
function(bifrost_add_external BIFROST_EXTERNAL)
  add_subdirectory("externals/${BIFROST_EXTERNAL}")
  set(BIFROST_EXTERNAL_FILE "externals/${BIFROST_EXTERNAL}/CMakeLists.txt")
  source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/externals/${BIFROST_EXTERNAL}"
    PREFIX "Externals\\${BIFROST_EXTERNAL}"
    FILES "${BIFROST_EXTERNAL_FILE}"
  )
  list(APPEND BIFROST_EXTERNAL_FILES "${BIFROST_EXTERNAL_FILE}")

  set(BIFROST_EXTERNAL_FILES ${BIFROST_EXTERNAL_FILES} PARENT_SCOPE)
endfunction()

file (GLOB SOURCE_FILES "src/*.cc") # Find all C++ files in the src directory
file (GLOB HEADER_FILES "include/*.hh") # Find all header files in the include directory
file (GLOB PYTHON_FILES "python/*.cc" "python/*.hh") # Find all the python bindings in the python directory
file (GLOB TEST_FILES "tests/cpp/*.cc") # Find all the test files in the test directory

include_directories(${CMAKE_CURRENT_LIST_DIR})


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

  ### Code block from: https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html
  include(FetchContent)

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  if(UNIX)
    SET(COMPILE_FLAGS "-g -O0 -coverage -fprofile-arcs -ftest-coverage")
  elseif(WIN32)
    SET(COMPILE_FLAGS "")
  endif()
  SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}" )

  set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
  # Try to set bindings for apple
  # Note I don't actually think that this works currently
  if(APPLE)
    find_package(Python 3 COMPONENTS Interpreter Development REQUIRED)
    find_package(SpiderMonkey REQUIRED)
    set(PYTHON_MAJOR $ENV{Python_VERSION_MAJOR})
    set(PYTHON_MINOR $ENV{Python_VERSION_MINOR})
    set(PYTHONLIBS_VERSION_STRING ${Python_VERSION})
    set(PYTHON_INCLUDE_DIR ${Python_INCLUDE_DIRS})
    set(PYTHON_LIBRARIES ${Python_LIBRARIES})
    message("Apple - Using Python:${Python_VERSION_MAJOR} - Libraries:${PYTHON_LIBRARIES} - IncludeDirs: ${PYTHON_INCLUDE_DIR}")
  elseif(UNIX)
    find_package(PythonLibs 3 REQUIRED)
    find_package(SpiderMonkey REQUIRED)
  elseif(WIN32)
    find_package(PythonInterp 3 REQUIRED)
    find_package(PythonLibs 3   REQUIRED)
    find_package(SpiderMonkey   REQUIRED)
    set(PYTHONLIBS_VERSION_STRING $ENV{PY_VERSION})
  endif()
  include_directories(${PYTHON_INCLUDE_DIRS})
  include_directories(${SPIDERMONKEY_INCLUDE_DIR})

  # Add doxygen if this is the main app
  find_package(Doxygen)
  if(Doxygen_FOUND)
      add_subdirectory(docs)
  else()
      message(STATUS "Doxygen not found. Not building docs.")
  endif()

  include(GoogleTest)

  add_subdirectory(tests)

endif()  

# Add compiled folder directories
add_subdirectory(src)

bifrost_add_external("uncrustify")
bifrost_add_external("autopep8")
add_subdirectory(format)