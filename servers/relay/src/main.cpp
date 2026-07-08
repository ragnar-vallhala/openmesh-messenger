#include "openmesh/core/logging.hpp"
#include "openmesh/core/version.hpp"

#include <string>

// Entry point for the optional relay server (SRS FR-8, §12).
//
// Skeleton only. The relay is deliberately dumb: it forwards encrypted packets
// between peers and keeps only temporary routing state. It must never be able to
// read payloads (SRS §8, §13).
//
// TODO:
//   * Load configuration (config/relay.example.toml).
//   * Bind a UDP socket and forward packets by routing envelope.
//   * Track and time out temporary routes.
int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    openmesh::core::log_info(std::string("openmesh-relay ") +
                             openmesh::core::kVersionString + " starting (skeleton)");
    return 0;
}
