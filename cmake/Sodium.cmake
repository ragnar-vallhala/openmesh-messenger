# Sodium.cmake — provide the `sodium::sodium` target for the crypto library,
# either from a vendored (pinned) source build or from the system installation.
#
# The choice is deliberately explicit so builds are reproducible:
#   * OPENMESH_VENDOR_SODIUM=ON  -> fetch and build a pinned libsodium (needs net
#                                   at configure time; reproducible & self-contained).
#   * OPENMESH_VENDOR_SODIUM=OFF -> use the system libsodium (default; fast).
#
# See third_party/README.md for how to vendor via a git submodule instead.

if(TARGET sodium::sodium)
    return()
endif()

option(OPENMESH_VENDOR_SODIUM
    "Build libsodium from a pinned vendored copy instead of the system package" OFF)

# Pinned version — bump deliberately (and note it in an ADR).
set(OPENMESH_SODIUM_VERSION "1.0.20" CACHE STRING "libsodium version to vendor")

if(OPENMESH_VENDOR_SODIUM)
    set(_vendored_dir "${CMAKE_SOURCE_DIR}/third_party/libsodium")
    if(EXISTS "${_vendored_dir}/CMakeLists.txt")
        # A CMake-enabled libsodium (e.g. the robinlinden/libsodium-cmake wrapper)
        # checked out as a submodule under third_party/libsodium.
        message(STATUS "libsodium: building vendored submodule at ${_vendored_dir}")
        add_subdirectory("${_vendored_dir}" "${CMAKE_BINARY_DIR}/_deps/sodium-build" EXCLUDE_FROM_ALL)
    else()
        # No submodule present — fetch a pinned, CMake-enabled libsodium.
        message(STATUS "libsodium: fetching pinned v${OPENMESH_SODIUM_VERSION} via FetchContent")
        include(FetchContent)
        FetchContent_Declare(sodium
            GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
            GIT_TAG e5b985ad0dd235d8c4307ea3a385b45e76c74a6a # pins libsodium 1.0.18-stable
            GIT_SUBMODULES_RECURSE ON)
        set(SODIUM_DISABLE_TESTS ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(sodium)
    endif()

    # Both paths above create a target named `sodium`; alias it to our canonical name.
    if(TARGET sodium AND NOT TARGET sodium::sodium)
        add_library(sodium::sodium ALIAS sodium)
    endif()
else()
    find_package(Sodium REQUIRED)
    if(SODIUM_INCLUDE_DIR)
        message(STATUS "libsodium: using system package (${PC_SODIUM_VERSION})")
    endif()
endif()

if(NOT TARGET sodium::sodium)
    message(FATAL_ERROR
        "libsodium not available. Install it (e.g. 'apt install libsodium-dev'), "
        "or configure with -DOPENMESH_VENDOR_SODIUM=ON to build a pinned copy.")
endif()
