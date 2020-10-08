set(PEGPARSER_SRC "${PROJECT_SOURCE_DIR}/ext/PEGParser")
set(EASYITERATOR_SRC "${PROJECT_SOURCE_DIR}/ext/EasyIterator")

FILE(GLOB_RECURSE headers CONFIGURE_DEPENDS "${PEGPARSER_SRC}/include/*.h" "${EASYITERATOR_SRC}/include/*.h")
FILE(GLOB_RECURSE sources CONFIGURE_DEPENDS "${PEGPARSER_SRC}/source/*.cpp")

add_library(PEGParser ${headers} ${sources})

target_include_directories(PEGParser PRIVATE
  ${PEGPARSER_SRC}/include
  ${EASYITERATOR_SRC}/include)

target_compile_options(PEGParser PRIVATE "-fPIC")

set_target_properties(PEGParser PROPERTIES CXX_STANDARD 17)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/peg_parser
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${PEGPARSER_SRC}/include" "include"
  DEPENDS PEGParser)

add_custom_target(pegparser_dep DEPENDS
  ${CMAKE_CURRENT_BINARY_DIR}/include/peg_parser)

set(PEGPARSER_TARGET pegparser_dep)
