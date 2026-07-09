# om-chat — the command-line client.
find_package(Threads REQUIRED)

# ncurses (wide) for the full-screen TUI.
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES REQUIRED IMPORTED_TARGET ncursesw)

add_executable(om-chat
    src/main.cpp
    src/tui.cpp
)
target_include_directories(om-chat PRIVATE src)
target_link_libraries(om-chat PRIVATE
    openmesh::messaging
    openmesh::net
    openmesh::crypto
    openmesh::storage
    openmesh::core
    PkgConfig::NCURSES
    Threads::Threads
    openmesh_warnings)
