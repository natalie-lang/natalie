set(HASHMAP_SRC "${PROJECT_SOURCE_DIR}/ext/hashmap")

include(ExternalProject)

ExternalProject_Add(hashmap
  EXCLUDE_FROM_ALL true
  BINARY_DIR ${HASHMAP_SRC}/build
  SOURCE_DIR ${HASHMAP_SRC}
  BUILD_COMMAND CFLAGS="-fPIC" make
  BUILD_BYPRODUCTS ${HASHMAP_SRC}/build/libhashmap.a
  INSTALL_COMMAND "")

set(HASHMAP_STATIC_LIB "${HASHMAP_SRC}/build/libhashmap.a")
add_library(libhashmap STATIC IMPORTED GLOBAL)
add_dependencies(libhashmap hashmap)
set_target_properties(libhashmap PROPERTIES IMPORTED_LOCATION ${HASHMAP_STATIC_LIB})
