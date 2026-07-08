add_executable(test_crypto test_identity.cpp)
target_link_libraries(test_crypto PRIVATE openmesh::crypto openmesh_warnings)
add_test(NAME crypto.identity COMMAND test_crypto)
