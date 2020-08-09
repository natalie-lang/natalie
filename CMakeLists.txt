cmake_minimum_required(VERSION 3.10)

project(Natalie)

add_library(natalie STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/generated/array.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/comparable.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/errno.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/exception.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/io.cpp
  src/array_value.cpp
  src/bindings.cpp
  src/block.cpp
  src/class_value.cpp
  src/encoding_value.cpp
  src/env.cpp
  src/env_value.cpp
  src/exception_value.cpp
  src/false_value.cpp
  src/file_value.cpp
  src/float_value.cpp
  src/hash_value.cpp
  src/integer_value.cpp
  src/io_value.cpp
  src/kernel_module.cpp
  src/match_data_value.cpp
  src/module_value.cpp
  src/natalie.cpp
  src/nil_value.cpp
  src/proc_value.cpp
  src/range_value.cpp
  src/regexp_value.cpp
  src/string_value.cpp
  src/symbol_value.cpp
  src/true_value.cpp
  src/value.cpp)

target_include_directories(natalie PRIVATE
  include
  ext/bdwgc/include
  ext/gdtoa
  ext/hashmap/include
  ext/onigmo)

set_target_properties(natalie PROPERTIES CXX_STANDARD 17 POSITION_INDEPENDENT_CODE true)

add_custom_target(make-generated-directory ALL COMMAND ${CMAKE_COMMAND} -E make_directory generated)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/natalie/compiler/binding_gen.rb
  COMMAND ruby ${CMAKE_CURRENT_SOURCE_DIR}/lib/natalie/compiler/binding_gen.rb > ${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp VERBATIM)
add_custom_target(generate-bindings DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp)
add_dependencies(natalie generate-bindings)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp PROPERTIES GENERATED TRUE)

add_custom_command(OUTPUT generated/array.cpp      DEPENDS src/array.rb      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --write-obj generated/array.cpp      ${CMAKE_CURRENT_SOURCE_DIR}/src/array.rb VERBATIM)
add_custom_command(OUTPUT generated/comparable.cpp DEPENDS src/comparable.rb COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --write-obj generated/comparable.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/comparable.rb VERBATIM)
add_custom_command(OUTPUT generated/errno.cpp      DEPENDS src/errno.rb      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --write-obj generated/errno.cpp      ${CMAKE_CURRENT_SOURCE_DIR}/src/errno.rb VERBATIM)
add_custom_command(OUTPUT generated/exception.cpp  DEPENDS src/exception.rb  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --write-obj generated/exception.cpp  ${CMAKE_CURRENT_SOURCE_DIR}/src/exception.rb VERBATIM)
add_custom_command(OUTPUT generated/io.cpp         DEPENDS src/io.rb         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --write-obj generated/io.cpp         ${CMAKE_CURRENT_SOURCE_DIR}/src/io.rb VERBATIM)
add_custom_target(compile-rb DEPENDS
  generated/array.cpp
  generated/comparable.cpp
  generated/errno.cpp
  generated/exception.cpp
  generated/io.cpp)
add_dependencies(natalie compile-rb)

set_source_files_properties(generated/array.cpp PROPERTIES GENERATED TRUE)
set_source_files_properties(generated/comparable.cpp PROPERTIES GENERATED TRUE)
set_source_files_properties(generated/errno.cpp PROPERTIES GENERATED TRUE)
set_source_files_properties(generated/exception.cpp PROPERTIES GENERATED TRUE)
set_source_files_properties(generated/io.cpp PROPERTIES GENERATED TRUE)

include(ext/bdwgc.cmake)
include(ext/gdtoa.cmake)
include(ext/hashmap.cmake)
include(ext/onigmo.cmake)
