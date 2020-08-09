set(GDTOA_SRC "${PROJECT_SOURCE_DIR}/ext/gdtoa")

include(ExternalProject)

ExternalProject_Add(gdtoa
  BUILD_IN_SOURCE TRUE
  SOURCE_DIR ${GDTOA_SRC}
  CONFIGURE_COMMAND ./autogen.sh
            COMMAND ./configure --with-pic
  BUILD_COMMAND $(MAKE) && rm -f compile)
