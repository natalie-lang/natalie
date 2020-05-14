cmake_minimum_required(VERSION 3.5)
project(hashmap VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

##############################################
# Create target and set properties

add_library(hashmap
    src/hashmap.c
)
set_property(TARGET hashmap PROPERTY POSITION_INDEPENDENT_CODE ON)

# Add an alias so that library can be used inside the build tree, e.g. when testing
add_library(HashMap::HashMap ALIAS hashmap)

# Set target properties
target_include_directories(hashmap
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(hashmap PRIVATE $<$<C_COMPILER_ID:GNU>:-Wall -Werror>)

##############################################
# Installation instructions

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/HashMap)

install(TARGETS hashmap
    EXPORT hashmap-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# This is required so that the exported target has the name HashMap and not hashmap
set_target_properties(hashmap PROPERTIES EXPORT_NAME HashMap)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export the targets to a script
install(EXPORT hashmap-targets
  FILE
    HashMapTargets.cmake
  NAMESPACE
    HashMap::
  DESTINATION
    ${INSTALL_CONFIGDIR}
)

# Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/HashMapConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/HashMapConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/HashMapConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

# Install the config, configversion and custom find modules
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/HashMapConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/HashMapConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)

##############################################
## Exporting from the build tree
export(EXPORT hashmap-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/HashMapTargets.cmake NAMESPACE HashMap::)

#Register package in user's package registry
export(PACKAGE HashMap)
