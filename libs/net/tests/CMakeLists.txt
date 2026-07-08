add_executable(test_net test_endpoint.cpp)
target_link_libraries(test_net PRIVATE openmesh::net openmesh_warnings)
add_test(NAME net.endpoint COMMAND test_net)

add_executable(test_udp test_udp_socket.cpp)
target_link_libraries(test_udp PRIVATE openmesh::net openmesh::protocol openmesh_warnings)
add_test(NAME net.udp_socket COMMAND test_udp)

add_executable(test_signaling_client test_signaling_client.cpp)
target_link_libraries(test_signaling_client PRIVATE openmesh::net openmesh_warnings)
add_test(NAME net.signaling_client COMMAND test_signaling_client)
