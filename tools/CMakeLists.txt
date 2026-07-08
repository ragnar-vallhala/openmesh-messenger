# Developer / CLI tools. Each is a small standalone executable that links the
# shared libraries — handy for testing the protocol and crypto without the GUI.

add_executable(om-keygen keygen/main.cpp)
target_link_libraries(om-keygen PRIVATE openmesh::crypto openmesh::core openmesh_warnings)

add_executable(om-simnet simnet/main.cpp)
target_link_libraries(om-simnet PRIVATE openmesh::net openmesh::protocol openmesh::core openmesh_warnings)

# End-to-end encrypted-message demonstration.
add_executable(om-chat-demo chatdemo/main.cpp)
target_link_libraries(om-chat-demo PRIVATE
    openmesh::messaging openmesh::crypto openmesh::protocol openmesh::net openmesh::core openmesh_warnings)
