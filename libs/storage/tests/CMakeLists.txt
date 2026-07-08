add_executable(test_storage test_store.cpp)
target_link_libraries(test_storage PRIVATE openmesh::storage openmesh_warnings)
add_test(NAME storage.store COMMAND test_storage)
