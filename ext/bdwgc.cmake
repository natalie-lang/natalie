set(BDWGC_SRC "${PROJECT_SOURCE_DIR}/ext/bdwgc")

include(ExternalProject)

ExternalProject_Add(bdwgc
  EXCLUDE_FROM_ALL true
  BUILD_IN_SOURCE TRUE
  SOURCE_DIR ${BDWGC_SRC}
  CONFIGURE_COMMAND ./autogen.sh
            COMMAND ./configure --enable-cplusplus --enable-redirect-malloc --disable-threads --enable-static --with-pic
  BUILD_COMMAND $(MAKE))

set(BDWGC_STATIC_LIB "${BDWGC_SRC}/.libs/libgccpp.a")
add_library(libbdwgc STATIC IMPORTED GLOBAL)
add_dependencies(libbdwgc bdwgc)
set_target_properties(libbdwgc PROPERTIES IMPORTED_LOCATION ${BDWGC_STATIC_LIB})
