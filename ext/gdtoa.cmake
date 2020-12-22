set(GDTOA_SRC "${PROJECT_SOURCE_DIR}/ext/gdtoa")

set(GDTOA_LIB "${CMAKE_BINARY_DIR}/libgdtoa.a")
set(GDTOA_BUILD_DIR "${CMAKE_BINARY_DIR}/gdtoa-build")

add_custom_target(make_gdtoa_build_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GDTOA_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/gdtoa)

add_custom_command(
    OUTPUT "${GDTOA_LIB}"
    BYPRODUCTS "${GDTOA_BUILD_DIR}/libgdtoa.a" "${GDTOA_SRC}/arith.h"
    DEPENDS make_gdtoa_build_dir
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${GDTOA_SRC}" "${GDTOA_BUILD_DIR}"
    WORKING_DIRECTORY ${GDTOA_BUILD_DIR}
    COMMAND sh autogen.sh
    COMMAND ./configure --with-pic
    COMMAND ${CMAKE_MAKE_PROGRAM}
    COMMAND ${CMAKE_COMMAND} -E copy "${GDTOA_BUILD_DIR}/*.h" "${CMAKE_BINARY_DIR}/include/gdtoa"
    COMMAND ${CMAKE_COMMAND} -E copy "${GDTOA_BUILD_DIR}/.libs/libgdtoa.a" "${GDTOA_LIB}")

add_custom_target(gdtoa
    DEPENDS "${GDTOA_LIB}" "${GDTOA_CPPLIB}")

set(GDTOA_TARGET gdtoa)
