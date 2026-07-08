add_executable(test_messaging test_engine.cpp)
target_link_libraries(test_messaging PRIVATE openmesh::messaging openmesh_warnings)
add_test(NAME messaging.engine COMMAND test_messaging)
