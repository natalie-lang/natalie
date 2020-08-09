set(ONIGMO_SRC "${PROJECT_SOURCE_DIR}/ext/onigmo")

include(ExternalProject)

ExternalProject_Add(onigmo
  EXCLUDE_FROM_ALL true
  BUILD_IN_SOURCE TRUE
  SOURCE_DIR ${ONIGMO_SRC}
  CONFIGURE_COMMAND ./configure --with-pic --disable-shared --enable-static
  BUILD_COMMAND $(MAKE))

set(ONIGMO_STATIC_LIB "${ONIGMO_SRC}/.libs/libonigmo.a")
add_library(libonigmo STATIC IMPORTED GLOBAL)
add_dependencies(libonigmo onigmo)
set_target_properties(libonigmo PROPERTIES IMPORTED_LOCATION ${ONIGMO_STATIC_LIB})
