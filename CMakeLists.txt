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

file (GLOB_RECURSE HEADER_FILES "include/*.hh") # Find all header files in the include directory
file (GLOB_RECURSE SOURCE_FILES "src/*.cc") # Find all C++ files in the src directory


include_directories(${CMAKE_CURRENT_LIST_DIR})


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

  ### Code block from: https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html
  include(FetchContent)

  SET(COMPILE_FLAGS "-fno-rtti -Wno-invalid-offsetof")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET(COMPILE_FLAGS "${COMPILE_FLAGS} -O0 -ggdb")
  else()
    SET(COMPILE_FLAGS "${COMPILE_FLAGS} -O3")
  endif()

  if(GENERATOR_IS_MULTI_CONFIG)
    string(APPEND COMPILE_FLAGS "$<$<CONFIG:DEBUG>:-O0 -ggdb> $<$<CONFIG:RELEASE>:-O3>")
  endif()

  SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")

  set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
  if(UNIX)
    find_package(Python 3.8 COMPONENTS Interpreter Development REQUIRED)
    set(Python_FIND_VIRTUALENV FIRST) # (require cmake >= v3.15 and this is the default) use the Python version configured by pyenv if available
    find_package(SpiderMonkey REQUIRED)
  elseif(WIN32)
    find_package(PythonInterp 3.8 REQUIRED)
    find_package(PythonLibs 3.8   REQUIRED)
    find_package(SpiderMonkey REQUIRED)
  endif()
  message("${CMAKE_SYSTEM_NAME} - Using Python:${Python_VERSION} - Libraries:${Python_LIBRARIES} - IncludeDirs: ${Python_INCLUDE_DIRS}")
  include_directories(${Python_INCLUDE_DIRS})
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
