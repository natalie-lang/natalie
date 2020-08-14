set(HASHMAP_SRC "${PROJECT_SOURCE_DIR}/ext/hashmap")

add_subdirectory(${HASHMAP_SRC})

target_compile_options(hashmap PRIVATE "-fPIC")

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libhashmap.a
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/ext/hashmap/libhashmap.a" "libhashmap.a"
  DEPENDS hashmap)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/hashmap.h
  COMMAND ${CMAKE_COMMAND} -E copy "${HASHMAP_SRC}/include/hashmap.h" "include/hashmap.h"
  DEPENDS hashmap)

add_custom_target(hashmap_dep DEPENDS
  ${CMAKE_CURRENT_BINARY_DIR}/include/hashmap.h
  ${CMAKE_CURRENT_BINARY_DIR}/libhashmap.a)

set(HASHMAP_TARGET hashmap_dep)
