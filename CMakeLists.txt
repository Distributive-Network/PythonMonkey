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

file (GLOB_RECURSE HEADER_FILES "include/*.hh") # Find all header files in the include directory and below
file (GLOB_RECURSE SOURCE_FILES "src/*.cc") # Find all C++ files in the src directory and below


include_directories(${CMAKE_CURRENT_LIST_DIR})


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

  ### Code block from: https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html
  include(FetchContent)

  SET(COMPILE_FLAGS "-fno-rtti -Wno-invalid-offsetof")

  SET(OPTIMIZED "-Ofast -DNDEBUG")
  SET(UNOPTIMIZED "-O0")
  SET(KEEP_SYMBOLS "-ggdb")
  SET(STRIP_SYMBOLS "-s")
  SET(PROFILE "-pg")

  SET(PROFILE_FLAGS  "${UNOPTIMIZED} ${KEEP_SYMBOLS} ${PROFILE}")
  SET(DEBUG_FLAGS    "${UNOPTIMIZED} ${KEEP_SYMBOLS}")
  SET(DRELEASE_FLAGS   "${OPTIMIZED} ${KEEP_SYMBOLS}")
  SET(RELEASE_FLAGS    "${OPTIMIZED} ${STRIP_SYMBOLS}")

  if(GENERATOR_IS_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES "Profile;Debug;DRelease;Release" CACHE STRING "" FORCE)
    string(APPEND COMPILE_FLAGS "$<$<CONFIG:Profile>:${PROFILE_FLAGS}> $<$<CONFIG:Debug>:${DEBUG_FLAGS}> $<$<CONFIG:DRelease>:${DRELEASE_FLAGS}> $<$<CONFIG:Release>:${RELEASE_FLAGS}>")
  else()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Profile;Debug;DRelease;Release")
    if(CMAKE_BUILD_TYPE STREQUAL "Profile")
      list(APPEND COMPILE_FLAGS "${PROFILE_FLAGS}")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
      list(APPEND COMPILE_FLAGS "${DEBUG_FLAGS}")
    elseif(CMAKE_BUILD_TYPE STREQUAL "DRelease")
      list(APPEND COMPILE_FLAGS "${DRELEASE_FLAGS}")
    else()  #Release build
      message("CMAKE_BUILD_TYPE not detected or invalid value, defaulting to Release build.")
      set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
      list(APPEND COMPILE_FLAGS "${RELEASE_FLAGS}") 
    endif()
    message("PythonMonkey build type is: ${CMAKE_BUILD_TYPE}")
    list(JOIN COMPILE_FLAGS " " COMPILE_FLAGS)
  endif()
  
  SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")

  set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
  if(APPLE)
    find_package(Python 3.8 COMPONENTS Interpreter Development REQUIRED)
    find_package(SpiderMonkey REQUIRED)
    set(PYTHON_MAJOR $ENV{Python_VERSION_MAJOR})
    set(PYTHON_MINOR $ENV{Python_VERSION_MINOR})
    set(PYTHONLIBS_VERSION_STRING ${Python_VERSION})
    set(PYTHON_INCLUDE_DIR ${Python_INCLUDE_DIRS})
    set(PYTHON_LIBRARIES ${Python_LIBRARIES})
  elseif(UNIX)
    find_package(Python 3.8 COMPONENTS Interpreter Development REQUIRED)
    set(Python_FIND_VIRTUALENV FIRST) # (require cmake >= v3.15 and this is the default) use the Python version configured by pyenv if available
    set(PYTHON_LIBRARIES ${Python_LIBRARIES})
    set(PYTHON_INCLUDE_DIR ${Python_INCLUDE_DIRS})
    find_package(SpiderMonkey REQUIRED)
  elseif(WIN32)
    find_package(PythonInterp 3.8 REQUIRED)
    find_package(PythonLibs 3.8   REQUIRED)
    find_package(SpiderMonkey REQUIRED)
    set(PYTHONLIBS_VERSION_STRING $ENV{PY_VERSION})
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
