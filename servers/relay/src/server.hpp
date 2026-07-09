#pragma once

#include "openmesh/net/endpoint.hpp"
#include "openmesh/net/udp_socket.hpp"
#include "router.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace openmesh::relay {

// One datagram to forward to a destination (raw bytes, verbatim).
struct Forward {
    net::Endpoint to;
    net::Bytes datagram;
};

// The optional relay server (SRS FR-8, §12). It forwards encrypted packets
// between peers that can't reach each other directly (e.g. both behind NAT):
// each peer only ever sends *outbound* to the relay, so NAT never blocks it.
//
// The relay is content-agnostic. It reads only the packet envelope — the source
// and destination public keys — never the (encrypted) payload:
//   * it records `source -> observed endpoint` from every packet (learning where
//     each peer is reachable, kept fresh by periodic announces);
//   * it forwards any packet with a `destination` to that peer's endpoint,
//     verbatim.
//
// The protocol logic (handle()) is I/O-free and unit-testable; bind()/run() wire
// it to a UDP socket.
//
// NOTE (v1): relay routes are unauthenticated — a peer is trusted to announce its
// own key. Since payloads are end-to-end encrypted, a hijacked route can only
// deny delivery, not read messages. A signed announce (like signaling REGISTER)
// is a hardening follow-up.
class RelayServer {
public:
    struct Config {
        std::int64_t route_ttl_seconds = 60; // forget a peer's route after this idle time
    };

    RelayServer();
    explicit RelayServer(Config config);

    // Process one received datagram: learn the source route and, if the packet is
    // addressed to a known peer, append a Forward. `now_seconds` is injected for
    // deterministic tests.
    void handle(const net::Bytes& datagram, const net::Endpoint& from, std::int64_t now_seconds,
                std::vector<Forward>& out);

    void expire(std::int64_t now_seconds);

    bool bind(const net::Endpoint& local);
    bool run_once(int timeout_ms, std::int64_t now_seconds);
    void run();
    void stop();

    [[nodiscard]] std::optional<net::Endpoint> local_endpoint() const;
    [[nodiscard]] const RouteTable& routes() const { return routes_; }

private:
    Config config_;
    RouteTable routes_;
    net::UdpSocket socket_;
    bool running_ = false;
};

} // namespace openmesh::relay
