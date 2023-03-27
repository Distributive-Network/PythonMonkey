include_directories(${CMAKE_CURRENT_LIST_DIR})

list(APPEND PYTHONMONKEY_SOURCE_FILES ${SOURCE_FILES} "${CMAKE_SOURCE_DIR}/src/modules/pythonmonkey/pythonmonkey.cc")

add_library(pythonmonkey SHARED
    ${PYTHONMONKEY_SOURCE_FILES}
)

execute_process(
    COMMAND python3-config --prefix
    OUTPUT_VARIABLE pyloc
)

target_include_directories(pythonmonkey PUBLIC ..)

if(WIN32)
  set_target_properties(
    pythonmonkey
    PROPERTIES
      PREFIX ""
      SUFFIX ".pyd"
      OUTPUT_NAME "pythonmonkey"
      CXX_STANDARD 17
  )
elseif(UNIX)
  set_target_properties(
    pythonmonkey
    PROPERTIES
    PREFIX ""
    SUFFIX ".so"
  )
endif()

target_link_libraries(pythonmonkey ${PYTHON_LIBRARIES})
target_link_libraries(pythonmonkey ${SPIDERMONKEY_LIBRARIES})

target_include_directories(pythonmonkey PRIVATE ${PYTHON_INCLUDE_DIR})
target_include_directories(pythonmonkey PRIVATE ${SPIDERMONKEY_INCLUDE_DIR})