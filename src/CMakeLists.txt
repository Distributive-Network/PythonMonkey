include_directories(${CMAKE_CURRENT_LIST_DIR})

file (GLOB SOURCE_FILES "../src/*.cpp") # Find all C++ files in the src directory
file (GLOB HEADER_FILES "../include/*.hpp") # Find all hpp files in the include directory

add_library(explore SHARED
    ${SOURCE_FILES}
) # Designate the source files as the library (to be compiled into a .so file)

execute_process(
    COMMAND python3-config --prefix
    OUTPUT_VARIABLE pyloc
)

target_include_directories(explore PUBLIC ..)

set_target_properties(explore PROPERTIES SUFFIX ".so")
set_target_properties(explore PROPERTIES PREFIX "")

target_include_directories(explore PRIVATE ${PYTHON_INCLUDE_DIR})
target_link_libraries(explore ${PYTHON_LIBRARIES})