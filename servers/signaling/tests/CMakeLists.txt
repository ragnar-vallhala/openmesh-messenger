add_executable(test_signaling
    test_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/registry.cpp)
target_include_directories(test_signaling PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_signaling PRIVATE openmesh::net openmesh_warnings)
add_test(NAME signaling.registry COMMAND test_signaling)
