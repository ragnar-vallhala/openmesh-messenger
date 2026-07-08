add_executable(test_signaling_registry test_registry.cpp)
target_link_libraries(test_signaling_registry PRIVATE signaling_core openmesh_warnings)
add_test(NAME signaling.registry COMMAND test_signaling_registry)

# Exercises the full protocol logic and a socket-level handshake; needs crypto
# to play the client side (sign the challenge).
add_executable(test_signaling_server test_server.cpp)
target_link_libraries(test_signaling_server
    PRIVATE signaling_core openmesh::crypto openmesh_warnings)
add_test(NAME signaling.server COMMAND test_signaling_server)
