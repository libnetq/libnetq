LNQ_FEATURE_DEFINE(ENABLE_JSON "Build JSON support")
LNQ_OPTION_DEFINE(JSON_BACKEND "Choose the library" ENUM AUTO JANSSON CJSON)

set(libnetq_HEADERS
  JSON.h
  JSONTypes.h
  JSONWriter.h
  )

set(libnetq_SOURCES
  cjson/JSON.c
  jansson/JSON.c
  stub/JSON.c
  JSON.c
  JSONWriter.c
  )

set(libnetq_INCLUDES
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )

if (LNQ_JSON_BACKEND STREQUAL "JANSSON")
  find_package(Jansson REQUIRED)
  list(APPEND libnetq_LIBRARIES Jansson::Jansson)
elseif (LNQ_JSON_BACKEND STREQUAL "CJSON")
  find_package(Jansson REQUIRED)
  list(APPEND libnetq_LIBRARIES CJSON::CJSON)
elseif (LNQ_JSON_BACKEND STREQUAL "AUTO")
  find_package(Jansson)
  find_package(CJSON)
  if (TARGET Jansson::Jansson)
    list(APPEND libnetq_LIBRARIES Jansson::Jansson)
  elseif (TARGET CJSON::CJSON)
    list(APPEND libnetq_LIBRARIES CJSON::CJSON)
  else ()
    message("! JSON has no implementation")
    message(FATAL_ERROR "No CJSON or Jansson library available")
  endif ()
else ()
endif ()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${libnetq_HEADERS} ${libnetq_SOURCES})
add_library(libnetq OBJECT ${libnetq_GENERATED} ${libnetq_HEADERS} ${libnetq_SOURCES})
target_include_directories(libnetq PRIVATE "${libnetq_INCLUDES}")
target_link_libraries(libnetq PUBLIC ${libnetq_LIBRARIES})

add_library(LibNetQ::JSON ALIAS libnetq)

install(FILES "${libnetq_HEADERS}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/json")
