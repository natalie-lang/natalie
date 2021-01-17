set(BDWGC_SRC "${PROJECT_SOURCE_DIR}/ext/bdwgc")

set(BDWGC_LIB "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libgc.a")
set(BDWGC_CPPLIB "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libgccpp.a")
set(BDWGC_BUILD_DIR "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/bdwgc-build")

add_custom_target(make_bdwgc_build_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BDWGC_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/bdwgc)

add_custom_command(
    OUTPUT "${BDWGC_LIB}" "${BDWGC_CPPLIB}"
    BYPRODUCTS "${BDWGC_BUILD_DIR}/libgc.a" "${BDWGC_BUILD_DIR}/libgccpp.a"
    DEPENDS make_bdwgc_build_dir
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${BDWGC_SRC}" "${BDWGC_BUILD_DIR}"
    WORKING_DIRECTORY ${BDWGC_BUILD_DIR}
    COMMAND [ -e .patched0 ] || (patch -sp0 < ../../../ext/bdwgc-openbsd.patch && touch .patched0)
    COMMAND sh autogen.sh
    COMMAND ./configure --enable-cplusplus --enable-threads=pthreads --enable-static --with-pic
    COMMAND ${CMAKE_MAKE_PROGRAM}
    COMMAND ${CMAKE_COMMAND} -E copy "${BDWGC_BUILD_DIR}/include/*.h" "${CMAKE_BINARY_DIR}/include/bdwgc"
    COMMAND ${CMAKE_COMMAND} -E copy "${BDWGC_BUILD_DIR}/.libs/libgc.a" "${BDWGC_LIB}"
    COMMAND ${CMAKE_COMMAND} -E copy "${BDWGC_BUILD_DIR}/.libs/libgccpp.a" "${BDWGC_CPPLIB}")

add_custom_target(bdwgc
    DEPENDS "${BDWGC_LIB}" "${BDWGC_CPPLIB}")

set(BDWGC_TARGET bdwgc)
