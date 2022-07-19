cmake_minimum_required(VERSION 3.4...3.18) # Set minimum cmake version
project(hello LANGUAGES CXX) # Give the project a name and declare that it is a C++ project

file (GLOB SOURCE_FILES "src/*.cpp") # Find all C++ files in the src directory
file (GLOB HEADER_FILES "include/*.hpp") # Find all hpp files in the include directory
file (GLOB PYTHON_FILES "python/*.cpp" "python/*.hpp") # Find all the python bindings in the python directory
file (GLOB TEST_FILES "test/cpp/*.cpp") # Find all the test files in the test directory

set(CMAKE_CXX_STANDARD 17) # Set the standard to C++17


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  # Try to set bindings for apple
  # Note I don't actually think that this works currently
  if(APPLE)
    find_package(Python 3 COMPONENTS Interpreter Development REQUIRED)
    set(PYTHON_MAJOR $ENV{Python_VERSION_MAJOR})
    set(PYTHON_MINOR $ENV{Python_VERSION_MINOR})
    set(PYTHONLIBS_VERSION_STRING ${Python_VERSION})
    set(PYTHON_INCLUDE_DIR ${Python_INCLUDE_DIRS})
    set(PYTHON_LIBRARIES ${Python_LIBRARIES})
    message("Apple - Using Python:${Python_VERSION_MAJOR} - Libraries:${PYTHON_LIBRARIES} - IncludeDirs: ${PYTHON_INCLUDE_DIR}")
  elseif(UNIX)
    find_package(PythonLibs 3 REQUIRED)
  else()
    set(PYTHONLIBS_VERSION_STRING $ENV{PY_VERSION})
  endif()
  # find_package(PythonLibs REQUIRED)
  include_directories(${PYTHON_INCLUDE_DIRS})

  # Add doxygen if this is the main app
  find_package(Doxygen)
  if(Doxygen_FOUND)
      add_subdirectory(docs)
  else()
      message(STATUS "Doxygen not found. Not building docs.")
  endif()

  include(GoogleTest)

  if(BUILD_TESTING)
      add_subdirectory(tests)
  endif()

endif()  

# Add compiled folder directories
add_subdirectory(src)

