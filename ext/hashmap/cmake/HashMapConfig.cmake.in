get_filename_component(HashMap_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

if(NOT TARGET HashMap::HashMap)
    include("${HashMap_CMAKE_DIR}/HashMapTargets.cmake")
endif()

set(HashMap_LIBRARIES HashMap::HashMap)
