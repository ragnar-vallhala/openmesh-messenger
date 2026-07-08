add_library(openmesh_crypto STATIC
    src/init.cpp
    src/identity.cpp
    src/sign.cpp
    src/aead.cpp
    src/key_agreement.cpp
    src/random.cpp
)
add_library(openmesh::crypto ALIAS openmesh_crypto)

target_include_directories(openmesh_crypto PUBLIC include)
target_link_libraries(openmesh_crypto
    PUBLIC  openmesh::core
    PRIVATE openmesh_warnings sodium::sodium)  # sodium stays an implementation detail

if(OPENMESH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
