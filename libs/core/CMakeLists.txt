add_library(openmesh_core STATIC
    src/logging.cpp
)
add_library(openmesh::core ALIAS openmesh_core)

target_include_directories(openmesh_core PUBLIC include)
target_link_libraries(openmesh_core PRIVATE openmesh_warnings)

if(OPENMESH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
