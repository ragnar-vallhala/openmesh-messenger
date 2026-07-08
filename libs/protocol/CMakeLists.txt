add_library(openmesh_protocol STATIC
    src/packet.cpp
)
add_library(openmesh::protocol ALIAS openmesh_protocol)

target_include_directories(openmesh_protocol PUBLIC include)
target_link_libraries(openmesh_protocol
    PUBLIC  openmesh::core
    PRIVATE openmesh_warnings)

if(OPENMESH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
