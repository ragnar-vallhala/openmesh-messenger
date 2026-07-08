add_executable(test_protocol test_packet.cpp)
target_link_libraries(test_protocol PRIVATE openmesh::protocol openmesh_warnings)
add_test(NAME protocol.packet_roundtrip COMMAND test_protocol)
