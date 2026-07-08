# Server logic as a small static library so the executable and the tests share it.
add_library(signaling_core STATIC
    src/registry.cpp
    src/server.cpp
)
target_include_directories(signaling_core PUBLIC src)
target_link_libraries(signaling_core
    PUBLIC  openmesh::core openmesh::protocol openmesh::net
    PRIVATE openmesh::crypto openmesh_warnings)  # crypto: proof of ownership (SRS §11)

add_executable(openmesh-signaling src/main.cpp)
target_link_libraries(openmesh-signaling PRIVATE signaling_core openmesh_warnings)

if(OPENMESH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
