add_library(openmesh_storage STATIC
    src/store.cpp
    src/identity_store.cpp
    src/blob.cpp
)
add_library(openmesh::storage ALIAS openmesh_storage)

target_include_directories(openmesh_storage PUBLIC include)
target_link_libraries(openmesh_storage
    PUBLIC  openmesh::core openmesh::protocol openmesh::crypto
    PRIVATE openmesh_warnings)

# TODO: link a local database backend (e.g. SQLite) once vendored.

if(OPENMESH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
