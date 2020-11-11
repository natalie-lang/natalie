set(BDWGC_SRC "${PROJECT_SOURCE_DIR}/ext/bdwgc")

set(BDWGC_LIB "${CMAKE_BINARY_DIR}/libgc.a")
set(BDWGC_CPPLIB "${CMAKE_BINARY_DIR}/libgccpp.a")
set(BDWGC_BUILD_DIR "${CMAKE_BINARY_DIR}/bdwgc-build")

add_custom_target(make_bdwgc_build_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BDWGC_BUILD_DIR})

add_custom_command(
    OUTPUT "${BDWGC_LIB}" "${BDWGC_CPPLIB}"
    BYPRODUCTS "${BDWGC_BUILD_DIR}/libgc.a" "${BDWGC_BUILD_DIR}/libgccpp.a"
    DEPENDS make_bdwgc_build_dir
    WORKING_DIRECTORY ${BDWGC_SRC}
    COMMAND sh autogen.sh
    COMMAND mkdir -p ${BDWGC_BUILD_DIR}
    COMMAND ./configure --enable-cplusplus --enable-redirect-malloc --enable-threads=pthreads --enable-static --with-pic --prefix "${BDWGC_BUILD_DIR}"
    COMMAND make
    COMMAND make install
    COMMAND make clean
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${BDWGC_BUILD_DIR}/include" "${CMAKE_BINARY_DIR}/include"
    COMMAND ${CMAKE_COMMAND} -E copy "${BDWGC_BUILD_DIR}/lib/libgc.a" "${BDWGC_LIB}"
    COMMAND ${CMAKE_COMMAND} -E copy "${BDWGC_BUILD_DIR}/lib/libgccpp.a" "${BDWGC_CPPLIB}")

add_custom_target(bdwgc
    DEPENDS "${BDWGC_LIB}" "${BDWGC_CPPLIB}")

set(BDWGC_TARGET bdwgc)
