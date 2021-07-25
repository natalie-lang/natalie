cmake_minimum_required(VERSION 3.10)

project(Natalie)

set(default_build_type "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "-pthread -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unknown-warning-option")
set(CMAKE_CXX_FLAGS_RELEASE "-pthread -g -O2")

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

add_custom_target(tests
    COMMAND ruby ${CMAKE_SOURCE_DIR}/test/all.rb
    USES_TERMINAL
)

file(GLOB LIB_SOURCES "src/*.cpp" "src/*.c")
file(GLOB GENERATED_NAMES "src/*.rb")

list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main.cpp$")

add_library(natalie-base STATIC
  ${LIB_SOURCES})

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated/bindings.cpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/natalie/compiler/binding_gen.rb
  DEPENDS make-generated-directory
  COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/lib/natalie/compiler/binding_gen.rb > ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated/bindings.cpp VERBATIM)
add_custom_target(generate-bindings DEPENDS ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated/bindings.cpp)
add_dependencies(natalie-base generate-bindings)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp PROPERTIES GENERATED TRUE)
target_sources(natalie-base PRIVATE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated/bindings.cpp)

foreach(PATH ${GENERATED_NAMES})
  get_filename_component(FILENAME ${PATH} NAME)
  string(REGEX REPLACE "[.]rb$" ".cpp" GENERATED_NAME ${FILENAME})
  set(GENERATED_PATH ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated/${GENERATED_NAME})
  add_custom_command(
    OUTPUT "${GENERATED_PATH}"
    DEPENDS "${PATH}"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/natalie/compiler/*
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/natalie/compiler.rb
    DEPENDS make-generated-directory
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --write-obj "${GENERATED_PATH}" ${PATH} VERBATIM)
  target_sources(natalie-base PRIVATE "${GENERATED_PATH}")
endforeach()

target_include_directories(natalie-base PRIVATE
  include
  ${CMAKE_BINARY_DIR}/include
  ${CMAKE_BINARY_DIR}/include/onigmo)

set_target_properties(natalie-base PROPERTIES CXX_STANDARD 17 POSITION_INDEPENDENT_CODE true)
set_property(TARGET natalie-base APPEND_STRING PROPERTY COMPILE_FLAGS $ENV{NAT_CXX_FLAGS})

add_custom_target(make-generated-directory ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated/platform.cpp
  DEPENDS make-generated-directory
  COMMAND ruby -e "puts %(#include \"natalie.hpp\"\\nconst char *Natalie::ruby_platform = #{RUBY_PLATFORM.inspect};)" > ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated/platform.cpp VERBATIM)
add_custom_target(generate-platform DEPENDS ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated/platform.cpp)
add_dependencies(natalie-base generate-platform)
set_source_files_properties(${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated/platform.cpp PROPERTIES GENERATED TRUE)
target_sources(natalie-base PRIVATE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/generated/platform.cpp)

include(ext/onigmo.cmake)

add_dependencies(natalie-base
  ${ONIGMO_TARGET})

file(GLOB_RECURSE HEADER_FILES "include/*.hpp")
add_custom_command(
  OUTPUT include/natalie.hpp
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}/include"
  DEPENDS ${HEADER_FILES})
add_custom_target(copy-includes ALL DEPENDS include/natalie.hpp)

add_custom_command(
  OUTPUT libnatalie.a
  COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libnatalie-o
    ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libonigmo-o
  COMMAND cd ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libnatalie-o && ar x ${CMAKE_BINARY_DIR}/libnatalie-base.a && ar r ${CMAKE_BINARY_DIR}/libnatalie.a *.o
  COMMAND cd ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libonigmo-o && ar x ../libonigmo.a && ar r ${CMAKE_BINARY_DIR}/libnatalie.a *.o
  DEPENDS natalie-base)
add_custom_target(combine-libraries ALL DEPENDS libnatalie.a)

file(GLOB_RECURSE RB_FILES "bin/*" "lib/*.rb")
add_custom_command(
  OUTPUT nat
  COMMAND ${CMAKE_SOURCE_DIR}/bin/natalie -c ${CMAKE_BINARY_DIR}/nat ${CMAKE_SOURCE_DIR}/bin/natalie
  DEPENDS ${RB_FILES} libnatalie.a)
add_custom_target(bootstrap DEPENDS nat)
