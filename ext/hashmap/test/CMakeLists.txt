cmake_minimum_required(VERSION 3.5)

add_executable(hashmap_test ../src/hashmap.c hashmap_test.c)
target_compile_options(hashmap_test PRIVATE $<$<C_COMPILER_ID:GNU>:-Wall -Werror>)
target_include_directories(hashmap_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Build hashmap library with metrics enabled
target_compile_definitions(hashmap_test PRIVATE HASHMAP_METRICS)
