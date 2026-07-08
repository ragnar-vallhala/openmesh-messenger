#pragma once

#include <cstdint>
#include <string>

namespace openmesh::net {

// A network endpoint (host + UDP port). Transport-agnostic value type.
struct Endpoint {
    std::string   host;
    std::uint16_t port = 0;

    [[nodiscard]] std::string to_string() const { return host + ':' + std::to_string(port); }
};

// How a peer connection was (or should be) established (SRS FR-7).
enum class ConnectPath { Direct, NatTraversal, Relay };

} // namespace openmesh::net
