add_executable(openmesh-signaling
    src/main.cpp
    src/registry.cpp
)

target_link_libraries(openmesh-signaling PRIVATE
    openmesh::core
    openmesh::protocol
    openmesh::crypto   # proof of key ownership (SRS §11) — never decrypts messages
    openmesh::net
    openmesh_warnings)

if(OPENMESH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
