# Minimal dependency-free tests using <cassert> + CTest.
# Swap for GoogleTest/Catch2 once a test framework is vendored in third_party/.

add_executable(test_core test_logging.cpp)
target_link_libraries(test_core PRIVATE openmesh::core openmesh_warnings)
add_test(NAME core.logging COMMAND test_core)
