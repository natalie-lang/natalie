set(ONIGMO_SRC "${PROJECT_SOURCE_DIR}/ext/onigmo")

set(ONIGMO_LIB "${CMAKE_BINARY_DIR}/libonigmo.a")
set(ONIGMO_BUILD_DIR "${ONIGMO_SRC}/build")

add_custom_target(make_onigmo_build_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ONIGMO_BUILD_DIR})

add_custom_command(
    OUTPUT "${ONIGMO_LIB}"
    BYPRODUCTS "${ONIGMO_BUILD_DIR}/libonigmo.a"
    DEPENDS make_onigmo_build_dir
    WORKING_DIRECTORY ${ONIGMO_SRC}
    COMMAND sh autogen.sh
    COMMAND mkdir -p build
    COMMAND ./configure --with-pic --prefix "${ONIGMO_BUILD_DIR}"
    COMMAND make
    COMMAND make install
    COMMAND make clean
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ONIGMO_BUILD_DIR}/include" "${CMAKE_BINARY_DIR}/include"
    COMMAND ${CMAKE_COMMAND} -E copy "${ONIGMO_BUILD_DIR}/lib/libonigmo.a" "${ONIGMO_LIB}")

add_custom_target(onigmo
    DEPENDS "${ONIGMO_LIB}")

set(ONIGMO_TARGET onigmo)
