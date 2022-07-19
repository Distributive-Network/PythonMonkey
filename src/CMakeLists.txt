add_library(hello SHARED
hello.cpp
) # Designate the source files as the library (to be compiled into a .so file)

target_include_directories(hello PRIVATE .)

target_include_directories(hello PRIVATE /opt/homebrew/opt/python@3.9/Frameworks/Python.framework/Versions/3.9/include/python3.9)
target_link_libraries(hello ${PYTHON_LIBRARIES})