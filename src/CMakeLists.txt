include_directories(${CMAKE_CURRENT_LIST_DIR})

list(APPEND EXPLORE_SOURCE_FILES  ${SOURCE_FILES} "${CMAKE_SOURCE_DIR}/src/modules/explore/explore.cc")
list(APPEND BIFROST2_SOURCE_FILES ${SOURCE_FILES} "${CMAKE_SOURCE_DIR}/src/modules/bifrost2/bifrost2.cc")

add_library(explore SHARED
    ${EXPLORE_SOURCE_FILES}
)

add_library(bifrost2 SHARED
    ${BIFROST2_SOURCE_FILES}
)

execute_process(
    COMMAND python3-config --prefix
    OUTPUT_VARIABLE pyloc
)

target_include_directories(explore  PUBLIC ..)
target_include_directories(bifrost2 PUBLIC ..)

if(WIN32)
  set_target_properties(
    explore
    PROPERTIES
      PREFIX ""
      SUFFIX ".pyd"
      OUTPUT_NAME "explore"
      CXX_STANDARD 17
  )
  set_target_properties(
    bifrost2
    PROPERTIES
      PREFIX ""
      SUFFIX ".pyd"
      OUTPUT_NAME "bifrost2"
      CXX_STANDARD 17
  )
elseif(UNIX)
  set_target_properties(
    explore
    PROPERTIES 
      PREFIX ""
      SUFFIX ".so"
  )
  set_target_properties(
    bifrost2
    PROPERTIES
    PREFIX ""
    SUFFIX ".so"
  )
endif()

target_include_directories(explore PRIVATE ${PYTHON_INCLUDE_DIR})
target_include_directories(bifrost2 PRIVATE ${PYTHON_INCLUDE_DIR})
target_include_directories(bifrost2 PRIVATE ${SPIDERMONKEY_INCLUDE_DIR})

target_link_libraries(explore ${PYTHON_LIBRARIES})
target_link_libraries(bifrost2 ${PYTHON_LIBRARIES})
target_link_libraries(bifrost2 ${SPIDERMONKEY_LIBRARIES})