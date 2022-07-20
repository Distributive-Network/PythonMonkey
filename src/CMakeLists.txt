add_library(test SHARED
hello.cpp
) # Designate the source files as the library (to be compiled into a .so file)

execute_process(
    COMMAND python3-config --prefix
    OUTPUT_VARIABLE pyloc
)

target_include_directories(test PRIVATE .)

set_target_properties(test PROPERTIES SUFFIX ".so")
set_target_properties(test PROPERTIES PREFIX "")

target_include_directories(test PRIVATE ${PYTHON_INCLUDE_DIR})
target_link_libraries(test ${PYTHON_LIBRARIES})