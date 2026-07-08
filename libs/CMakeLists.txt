# Shared libraries. Order matters only for readability; CMake resolves the
# actual dependency graph from target_link_libraries().
#
# Dependency overview:
#   core       <- (no openmesh deps)
#   protocol   <- core
#   crypto     <- core
#   net        <- core, protocol
#   storage    <- core, protocol, crypto
#   messaging  <- core, protocol, crypto, net, storage

add_subdirectory(core)
add_subdirectory(protocol)
add_subdirectory(crypto)
add_subdirectory(net)
add_subdirectory(storage)
add_subdirectory(messaging)
