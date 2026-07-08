add_library(openmesh_net STATIC
    src/transport.cpp
    src/udp_socket.cpp
)
add_library(openmesh::net ALIAS openmesh_net)

target_include_directories(openmesh_net PUBLIC include)
target_link_libraries(openmesh_net
    PUBLIC  openmesh::core openmesh::protocol
    PRIVATE openmesh_warnings)

if(OPENMESH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
