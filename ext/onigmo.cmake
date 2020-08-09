set(ONIGMO_SRC "${PROJECT_SOURCE_DIR}/ext/onigmo")

include(ExternalProject)

ExternalProject_Add(onigmo
  BUILD_IN_SOURCE TRUE
  SOURCE_DIR ${ONIGMO_SRC}
  CONFIGURE_COMMAND ./configure --with-pic --disable-shared --enable-static
  BUILD_COMMAND $(MAKE))
