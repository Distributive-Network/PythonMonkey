include(FetchContent)

include_directories(${CMAKE_CURRENT_LIST_DIR})

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tests ${TEST_FILES} ${SOURCE_FILES})

target_include_directories(tests PRIVATE ${PYTHON_INCLUDE_DIR} ${SPIDERMONKEY_INCLUDE_DIR})
target_include_directories(tests PRIVATE ../include/)
target_link_libraries(tests PRIVATE pythonmonkey gtest_main ${PYTHON_LIBRARIES} ${SPIDERMONKEY_LIBRARIES})

# Tell macOS where to find and load '@rpath/Python3.framework/Versions/3.9/Python3'
if(APPLE)
  target_link_options(tests PRIVATE "SHELL:-rpath /Library/Developer/CommandLineTools/Library/Frameworks/")
endif()

include(GoogleTest)
gtest_discover_tests(tests)

# add_test (NAME python-tests
#   COMMAND python3 -m pytest ${CMAKE_CURRENT_SOURCE_DIR}
#   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/python
# )