#include "registry.hpp"

#include "openmesh/core/logging.hpp"
#include "openmesh/core/version.hpp"

#include <string>

// Entry point for the volunteer signaling server (SRS FR-6, §11).
//
// Skeleton only: parses no real arguments and opens no sockets yet. It exists so
// the build produces a runnable binary and the registry can be wired in.
//
// TODO:
//   * Load configuration (config/signaling.example.toml).
//   * Bind a UDP socket and run the packet loop (REGISTER/DISCOVER/CONNECT).
//   * Verify proof of key ownership on REGISTER (SRS §11) via openmesh::crypto.
//   * Periodically expire inactive peers.
int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    openmesh::core::log_info(std::string("openmesh-signaling ") +
                             openmesh::core::kVersionString + " starting (skeleton)");

    openmesh::signaling::PeerRegistry registry;
    openmesh::core::log_info("peer registry ready; " +
                             std::to_string(registry.size()) + " peers");

    // No event loop yet.
    return 0;
}
