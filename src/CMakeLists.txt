include_directories(${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
if(APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path")
elseif(UNIX)
  set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

list(APPEND PYTHONMONKEY_SOURCE_FILES ${SOURCE_FILES} "${CMAKE_SOURCE_DIR}/src/modules/pythonmonkey/pythonmonkey.cc")

add_library(pythonmonkey SHARED
    ${PYTHONMONKEY_SOURCE_FILES}
)

target_include_directories(pythonmonkey PUBLIC ..)
target_compile_definitions(pythonmonkey PRIVATE BUILD_TYPE="${PM_BUILD_TYPE} $<CONFIG>")

if(WIN32)
  set_target_properties(
    pythonmonkey
    PROPERTIES
      PREFIX ""
      SUFFIX ".pyd"
      OUTPUT_NAME "pythonmonkey"
  )
elseif(UNIX)
  set_target_properties(
    pythonmonkey
    PROPERTIES
    PREFIX ""
    SUFFIX ".so"
  )
endif()

# Don't link against `libpython` on Unix, see https://peps.python.org/pep-0513/#libpythonx-y-so-1
if (WIN32) # linker error on Windows: `lld-link: could not open 'python311.lib': no such file or directory`
  target_link_libraries(pythonmonkey ${PYTHON_LIBRARIES})
endif()

# Stop complaining about missing symbols
# see https://blog.tim-smith.us/2015/09/python-extension-modules-os-x/
if(APPLE)
  target_link_options(pythonmonkey PRIVATE "SHELL:-undefined dynamic_lookup")
endif()

target_link_libraries(pythonmonkey ${SPIDERMONKEY_LIBRARIES})

target_include_directories(pythonmonkey PRIVATE ${PYTHON_INCLUDE_DIR})
target_include_directories(pythonmonkey PRIVATE ${SPIDERMONKEY_INCLUDE_DIR})
