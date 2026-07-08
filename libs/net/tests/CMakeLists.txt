add_executable(test_net test_endpoint.cpp)
target_link_libraries(test_net PRIVATE openmesh::net openmesh_warnings)
add_test(NAME net.endpoint COMMAND test_net)
