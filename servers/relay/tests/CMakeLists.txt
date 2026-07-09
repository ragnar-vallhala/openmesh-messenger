add_executable(test_relay_router test_router.cpp)
target_link_libraries(test_relay_router PRIVATE relay_core openmesh_warnings)
add_test(NAME relay.router COMMAND test_relay_router)

add_executable(test_relay_server test_server.cpp)
target_link_libraries(test_relay_server PRIVATE relay_core openmesh::protocol openmesh_warnings)
add_test(NAME relay.server COMMAND test_relay_server)

# End-to-end: two messaging Engines exchange encrypted messages through a real
# RelayServer over UDP loopback (both peers only ever send to the relay).
add_executable(test_relay_e2e test_e2e.cpp)
target_link_libraries(test_relay_e2e
    PRIVATE relay_core openmesh::messaging openmesh::crypto openmesh_warnings)
add_test(NAME relay.e2e COMMAND test_relay_e2e)
