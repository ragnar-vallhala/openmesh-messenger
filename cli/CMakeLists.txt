# om-chat — the command-line client.
find_package(Threads REQUIRED)

add_executable(om-chat src/main.cpp)
target_link_libraries(om-chat PRIVATE
    openmesh::messaging
    openmesh::net
    openmesh::crypto
    openmesh::storage
    openmesh::core
    Threads::Threads
    openmesh_warnings)
