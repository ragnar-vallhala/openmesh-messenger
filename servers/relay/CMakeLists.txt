add_executable(openmesh-relay
    src/main.cpp
)

target_link_libraries(openmesh-relay PRIVATE
    openmesh::core
    openmesh::protocol   # envelope only — never decrypts payloads
    openmesh::net
    openmesh_warnings)
