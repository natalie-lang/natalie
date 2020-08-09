set(HASHMAP_SRC "${PROJECT_SOURCE_DIR}/ext/hashmap")

include(ExternalProject)

ExternalProject_Add(hashmap
  BINARY_DIR ${HASHMAP_SRC}/build
  SOURCE_DIR ${HASHMAP_SRC}
  BUILD_COMMAND CFLAGS="-fPIC" make
  BUILD_BYPRODUCTS ${HASHMAP_SRC}/build/libhashmap.a
  INSTALL_COMMAND "")
