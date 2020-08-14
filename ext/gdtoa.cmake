set(GDTOA_SRC "${PROJECT_SOURCE_DIR}/ext/gdtoa")

set(GDTOA_LIB "${CMAKE_BINARY_DIR}/libgdtoa.a")
set(GDTOA_BUILD_DIR "${GDTOA_SRC}/build")

add_custom_target(make_gdtoa_build_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GDTOA_BUILD_DIR})

add_custom_command(
    OUTPUT "${GDTOA_LIB}"
    BYPRODUCTS "${GDTOA_BUILD_DIR}/libgdtoa.a" "${GDTOA_SRC}/arith.h"
    DEPENDS make_gdtoa_build_dir
    WORKING_DIRECTORY ${GDTOA_SRC}
    COMMAND sh autogen.sh
    COMMAND mkdir -p build
    COMMAND ./configure --with-pic --prefix "${GDTOA_BUILD_DIR}"
    COMMAND make
    COMMAND make install
    COMMAND ${CMAKE_COMMAND} -E copy "${GDTOA_BUILD_DIR}/lib/libgdtoa.a" "${GDTOA_LIB}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${GDTOA_BUILD_DIR}/include" "${CMAKE_BINARY_DIR}/include"
    COMMAND ${CMAKE_COMMAND} -E copy "${GDTOA_SRC}/*.h" "${CMAKE_BINARY_DIR}/include/gdtoa" # gdtoa does not install all its headers :(
    COMMAND make clean
    )

add_custom_target(gdtoa
    DEPENDS "${GDTOA_LIB}" "${GDTOA_CPPLIB}")

set(GDTOA_TARGET gdtoa)
