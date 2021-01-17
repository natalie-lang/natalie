cmake_minimum_required(VERSION 3.10)

project(Natalie)

set(default_build_type "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "-pthread -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unknown-warning-option")
set(CMAKE_CXX_FLAGS_RELEASE "-pthread -O2")
if(LINUX)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ldl")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ldl")
endif()

add_custom_target(tests
    COMMAND ruby ${CMAKE_SOURCE_DIR}/test/all.rb
    USES_TERMINAL
)

file(GLOB LIB_SOURCES "src/*.cpp")
file(GLOB GENERATED_NAMES "src/*.rb")

list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main.cpp$")

add_library(natalie-static STATIC
  ${LIB_SOURCES})

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/generated/bindings.cpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/natalie/compiler/binding_gen.rb
  DEPENDS make-generated-directory
  COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/lib/natalie/compiler/binding_gen.rb > ${CMAKE_BINARY_DIR}/generated/bindings.cpp VERBATIM)
add_custom_target(generate-bindings DEPENDS ${CMAKE_BINARY_DIR}/generated/bindings.cpp)
add_dependencies(natalie-static generate-bindings)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp PROPERTIES GENERATED TRUE)
target_sources(natalie-static PRIVATE ${CMAKE_BINARY_DIR}/generated/bindings.cpp)

foreach(PATH ${GENERATED_NAMES})
  get_filename_component(FILENAME ${PATH} NAME)
  string(REGEX REPLACE "[.]rb$" ".cpp" GENERATED_NAME ${FILENAME})
  set(GENERATED_PATH generated/${GENERATED_NAME})
  add_custom_command(
    OUTPUT "${GENERATED_PATH}"
    DEPENDS "${PATH}"
    DEPENDS make-generated-directory
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --write-obj "${GENERATED_PATH}" ${PATH} VERBATIM)
  target_sources(natalie-static PRIVATE "${GENERATED_PATH}")
endforeach()

target_include_directories(natalie-static PRIVATE
  include
  ${CMAKE_BINARY_DIR}/include
  ${CMAKE_BINARY_DIR}/include/bdwgc
  ${CMAKE_BINARY_DIR}/include/gdtoa
  ${CMAKE_BINARY_DIR}/include/onigmo)

set_target_properties(natalie-static PROPERTIES CXX_STANDARD 17 POSITION_INDEPENDENT_CODE true)
set_property(TARGET natalie-static APPEND_STRING PROPERTY COMPILE_FLAGS $ENV{NAT_CXX_FLAGS})

add_custom_target(make-generated-directory ALL COMMAND ${CMAKE_COMMAND} -E make_directory generated)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/generated/platform.cpp
  DEPENDS make-generated-directory
  COMMAND ruby -e "puts %(#include \"natalie.hpp\"\\nconst char *Natalie::ruby_platform = #{RUBY_PLATFORM.inspect};)" > ${CMAKE_BINARY_DIR}/generated/platform.cpp VERBATIM)
add_custom_target(generate-platform DEPENDS ${CMAKE_BINARY_DIR}/generated/platform.cpp)
add_dependencies(natalie-static generate-platform)
set_source_files_properties(${CMAKE_BINARY_DIR}/generated/platform.cpp PROPERTIES GENERATED TRUE)
target_sources(natalie-static PRIVATE ${CMAKE_BINARY_DIR}/generated/platform.cpp)

include(ext/bdwgc.cmake)
include(ext/gdtoa.cmake)
include(ext/onigmo.cmake)

add_dependencies(natalie-static
  ${BDWGC_TARGET}
  ${GDTOA_TARGET}
  ${ONIGMO_TARGET})

add_custom_command(
  OUTPUT libnatalie.a
  COMMAND ${CMAKE_COMMAND} -E make_directory libnatalie-o libgc-o libgccpp-o libgdtoa-o libonigmo-o
  COMMAND cd libnatalie-o && ar x ../libnatalie-static.a && ar r ../libnatalie.a *.o
  COMMAND cd libgc-o && ar x ../libgc.a && ar r ../libnatalie.a *.o
  COMMAND cd libgccpp-o && ar x ../libgccpp.a && ar r ../libnatalie.a *.o
  COMMAND cd libgdtoa-o && ar x ../libgdtoa.a && ar r ../libnatalie.a *.o
  COMMAND cd libonigmo-o && ar x ../libonigmo.a && ar r ../libnatalie.a *.o
  DEPENDS natalie-static)

add_custom_target(full-library ALL DEPENDS libnatalie.a)
