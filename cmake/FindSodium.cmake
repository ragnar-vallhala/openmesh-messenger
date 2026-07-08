# FindSodium.cmake — locate a system/prebuilt libsodium and expose it as the
# imported target `sodium::sodium`.
#
# Used by cmake/Sodium.cmake when OPENMESH_VENDOR_SODIUM is OFF (the default).
# Prefers pkg-config, then falls back to standard search paths.

if(TARGET sodium::sodium)
    return()
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_SODIUM QUIET libsodium)
endif()

find_path(SODIUM_INCLUDE_DIR
    NAMES sodium.h
    HINTS ${PC_SODIUM_INCLUDEDIR} ${PC_SODIUM_INCLUDE_DIRS})

find_library(SODIUM_LIBRARY
    NAMES sodium libsodium
    HINTS ${PC_SODIUM_LIBDIR} ${PC_SODIUM_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sodium
    REQUIRED_VARS SODIUM_LIBRARY SODIUM_INCLUDE_DIR
    VERSION_VAR PC_SODIUM_VERSION)

if(Sodium_FOUND AND NOT TARGET sodium::sodium)
    add_library(sodium::sodium UNKNOWN IMPORTED)
    set_target_properties(sodium::sodium PROPERTIES
        IMPORTED_LOCATION "${SODIUM_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SODIUM_INCLUDE_DIR}")
endif()

mark_as_advanced(SODIUM_INCLUDE_DIR SODIUM_LIBRARY)
