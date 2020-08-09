set(GDTOA_SRC "${PROJECT_SOURCE_DIR}/ext/gdtoa")

include(ExternalProject)

ExternalProject_Add(gdtoa
  EXCLUDE_FROM_ALL true
  BUILD_IN_SOURCE TRUE
  SOURCE_DIR ${GDTOA_SRC}
  CONFIGURE_COMMAND ./autogen.sh
            COMMAND ./configure --with-pic
  BUILD_COMMAND $(MAKE) && rm -f compile)

set(GDTOA_STATIC_LIB "${GDTOA_SRC}/.libs/libgdtoa.a")
add_library(libgdtoa STATIC IMPORTED GLOBAL)
add_dependencies(libgdtoa gdtoa)
set_target_properties(libgdtoa PROPERTIES IMPORTED_LOCATION ${GDTOA_STATIC_LIB})
