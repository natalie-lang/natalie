set(BDWGC_SRC "${PROJECT_SOURCE_DIR}/ext/bdwgc")

include(ExternalProject)

ExternalProject_Add(bdwgc
  BUILD_IN_SOURCE TRUE
  SOURCE_DIR ${BDWGC_SRC}
  CONFIGURE_COMMAND ./autogen.sh
            COMMAND ./configure --enable-cplusplus --enable-redirect-malloc --disable-threads --enable-static --with-pic
  BUILD_COMMAND $(MAKE))
