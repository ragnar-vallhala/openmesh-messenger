# Relay logic as a small static library so the executable and tests share it.
add_library(relay_core STATIC
    src/router.cpp
    src/server.cpp
)
target_include_directories(relay_core PUBLIC src)
target_link_libraries(relay_core
    PUBLIC  openmesh::core openmesh::protocol openmesh::net
    PRIVATE openmesh_warnings) # content-agnostic — no crypto dependency

add_executable(openmesh-relay src/main.cpp)
target_link_libraries(openmesh-relay PRIVATE relay_core openmesh_warnings)

if(OPENMESH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
