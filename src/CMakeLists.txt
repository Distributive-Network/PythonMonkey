add_library(explore SHARED
explore.cpp
../include/explore.hpp
) # Designate the source files as the library (to be compiled into a .so file)

execute_process(
    COMMAND python3-config --prefix
    OUTPUT_VARIABLE pyloc
)

target_include_directories(explore PRIVATE .)

set_target_properties(explore PROPERTIES SUFFIX ".so")
set_target_properties(explore PROPERTIES PREFIX "")

target_include_directories(explore PRIVATE ${PYTHON_INCLUDE_DIR})
target_link_libraries(explore ${PYTHON_LIBRARIES})