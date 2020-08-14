cmake_minimum_required(VERSION 3.10)

project(Natalie)

file(GLOB LIB_SOURCES "src/*.cpp")
file(GLOB GENERATED_NAMES "src/*.rb")

list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main.cpp$")
list(APPEND LIB_SOURCES "src/bindings.cpp")

add_library(natalie STATIC
  ${LIB_SOURCES})

foreach(PATH ${GENERATED_NAMES})
  get_filename_component(FILENAME ${PATH} NAME)
  string(REGEX REPLACE "[.]rb$" ".cpp" GENERATED_NAME ${FILENAME})
  set(GENERATED_PATH generated/${GENERATED_NAME})
  add_custom_command(
    OUTPUT "${GENERATED_PATH}"
    DEPENDS "${PATH}"
    DEPENDS make-generated-directory
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --write-obj "${GENERATED_PATH}" ${PATH} VERBATIM)
  target_sources(natalie PRIVATE "${GENERATED_PATH}")
endforeach()

target_include_directories(natalie PRIVATE
  include
  ${CMAKE_BINARY_DIR}/include
  ${CMAKE_BINARY_DIR}/include/gdtoa)

set_target_properties(natalie PROPERTIES CXX_STANDARD 17 POSITION_INDEPENDENT_CODE true)

add_custom_target(make-generated-directory ALL COMMAND ${CMAKE_COMMAND} -E make_directory generated)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/natalie/compiler/binding_gen.rb
  COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/lib/natalie/compiler/binding_gen.rb > ${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp VERBATIM)
add_custom_target(generate-bindings DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp)
add_dependencies(natalie generate-bindings)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp PROPERTIES GENERATED TRUE)

include(ext/bdwgc.cmake)
include(ext/gdtoa.cmake)
include(ext/hashmap.cmake)
include(ext/onigmo.cmake)

add_dependencies(natalie
    ${BDWGC_TARGET}
    ${GDTOA_TARGET}
    ${HASHMAP_TARGET}
    ${ONIGMO_TARGET})
