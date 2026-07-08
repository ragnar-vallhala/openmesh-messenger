# Tests need crypto directly to mint identities for the two peers.
add_executable(test_session test_session.cpp)
target_link_libraries(test_session PRIVATE openmesh::messaging openmesh::crypto openmesh_warnings)
add_test(NAME messaging.session COMMAND test_session)

add_executable(test_messaging test_engine.cpp)
target_link_libraries(test_messaging PRIVATE openmesh::messaging openmesh::crypto openmesh_warnings)
add_test(NAME messaging.engine COMMAND test_messaging)
