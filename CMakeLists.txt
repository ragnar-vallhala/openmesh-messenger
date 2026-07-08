cmake_minimum_required(VERSION 3.21)

project(OpenMeshMessenger
    VERSION 0.1.0
    DESCRIPTION "Decentralized, end-to-end encrypted messenger"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Export compile_commands.json for clangd / tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ---------------------------------------------------------------------------
# Options: choose which components to build
# ---------------------------------------------------------------------------
option(OPENMESH_BUILD_CLIENT  "Build the Qt/QML desktop & Android client" ON)
option(OPENMESH_BUILD_SERVERS "Build the signaling and relay servers"      ON)
option(OPENMESH_BUILD_TOOLS   "Build developer/CLI tools"                  ON)
option(OPENMESH_BUILD_TESTS   "Build unit tests"                           ON)

include(OpenMeshWarnings)

# Cryptographic backend (libsodium). Provides the sodium::sodium target.
include(Sodium)

if(OPENMESH_BUILD_TESTS)
    enable_testing()
endif()

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
# Shared libraries used by both the client and the servers.
add_subdirectory(libs)

if(OPENMESH_BUILD_SERVERS)
    add_subdirectory(servers/signaling)
    add_subdirectory(servers/relay)
endif()

if(OPENMESH_BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(OPENMESH_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

message(STATUS "OpenMesh Messenger ${PROJECT_VERSION}")
message(STATUS "  client : ${OPENMESH_BUILD_CLIENT}")
message(STATUS "  servers: ${OPENMESH_BUILD_SERVERS}")
message(STATUS "  tools  : ${OPENMESH_BUILD_TOOLS}")
message(STATUS "  tests  : ${OPENMESH_BUILD_TESTS}")
