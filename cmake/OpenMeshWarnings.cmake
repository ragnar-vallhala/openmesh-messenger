# Shared compiler-warning configuration.
#
# Usage:  target_link_libraries(my_target PRIVATE openmesh_warnings)
#
# Kept as an INTERFACE library so every target opts in the same way without
# polluting global flags.

if(TARGET openmesh_warnings)
    return()
endif()

add_library(openmesh_warnings INTERFACE)

if(MSVC)
    target_compile_options(openmesh_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(openmesh_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wnon-virtual-dtor)
endif()

option(OPENMESH_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
if(OPENMESH_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(openmesh_warnings INTERFACE /WX)
    else()
        target_compile_options(openmesh_warnings INTERFACE -Werror)
    endif()
endif()
