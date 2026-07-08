add_library(openmesh_messaging STATIC
    src/engine.cpp
)
add_library(openmesh::messaging ALIAS openmesh_messaging)

target_include_directories(openmesh_messaging PUBLIC include)
target_link_libraries(openmesh_messaging
    PUBLIC  openmesh::core openmesh::protocol openmesh::crypto
            openmesh::net openmesh::storage
    PRIVATE openmesh_warnings)

if(OPENMESH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
