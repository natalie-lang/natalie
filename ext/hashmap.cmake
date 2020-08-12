set(HASHMAP_SRC "${PROJECT_SOURCE_DIR}/ext/hashmap")

add_subdirectory(${HASHMAP_SRC})

target_compile_options(hashmap PRIVATE "-fPIC")

add_custom_target(hashmap_dep
    COMMAND ${CMAKE_COMMAND} -E copy "ext/hashmap/libhashmap.a" "libhashmap.a"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${HASHMAP_SRC}/include" "include"
    DEPENDS hashmap)

set(HASHMAP_TARGET hashmap_dep)
