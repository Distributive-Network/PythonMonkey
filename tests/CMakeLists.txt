include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(testlib testlib.cpp)

target_link_libraries(testlib PRIVATE gtest_main)

include(GoogleTest)
gtest_discover_tests(testlib)