add_library(openmesh_crypto STATIC
    src/identity.cpp
)
add_library(openmesh::crypto ALIAS openmesh_crypto)

target_include_directories(openmesh_crypto PUBLIC include)
target_link_libraries(openmesh_crypto
    PUBLIC  openmesh::core
    PRIVATE openmesh_warnings)

# TODO: link a vetted crypto backend (libsodium recommended) once vendored:
#   find_package(unofficial-sodium CONFIG REQUIRED)
#   target_link_libraries(openmesh_crypto PRIVATE unofficial-sodium::sodium)

if(OPENMESH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
