#pragma once

#include "openmesh/net/endpoint.hpp"
#include "openmesh/net/udp_socket.hpp"
#include "openmesh/protocol/packet.hpp"
#include "registry.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace openmesh::signaling {

// A packet the server wants to send in response to an incoming one.
struct Outgoing {
    net::Endpoint to;
    protocol::Packet packet;
};

// The volunteer signaling server (SRS FR-6, §11).
//
// The protocol logic (handle()) is deliberately separated from I/O: it is a pure
// function of (incoming packet, source endpoint, time) -> outgoing packets +
// registry mutations, so it can be unit-tested without sockets. bind()/run()
// wire it to a real UdpSocket.
//
// Registration uses a proof-of-ownership handshake (SRS §11):
//   1. client -> REGISTER (src = Ed25519 public key, empty payload)
//   2. server -> HELLO    (payload = random challenge nonce)
//   3. client -> REGISTER (src = public key, payload = Ed25519 signature of nonce)
//   4. server verifies, registers the peer at its *observed* source endpoint,
//      and replies ACK. See docs/protocol/signaling.md.
class SignalingServer {
public:
    struct Config {
        std::int64_t peer_ttl_seconds = 120;     // forget peers idle this long (§11)
        std::int64_t challenge_ttl_seconds = 30; // pending challenge lifetime
    };

    SignalingServer();
    explicit SignalingServer(Config config);

    // --- Pure protocol logic (no I/O) ---------------------------------------
    // Process one received packet; append any responses to `out` and mutate the
    // registry. `now_seconds` is injected so tests are deterministic.
    void handle(const protocol::Packet& in, const net::Endpoint& from, std::int64_t now_seconds,
                std::vector<Outgoing>& out);

    // Drop expired peers and pending challenges.
    void expire(std::int64_t now_seconds);

    // --- I/O ----------------------------------------------------------------
    bool bind(const net::Endpoint& local);
    // Receive at most one datagram (waiting up to timeout_ms), handle it, send
    // the responses, then run expiry. Returns true if a packet was processed.
    bool run_once(int timeout_ms, std::int64_t now_seconds);
    void run(); // blocking loop until stop()
    void stop();

    [[nodiscard]] std::optional<net::Endpoint> local_endpoint() const;
    [[nodiscard]] const PeerRegistry& registry() const { return registry_; }

private:
    void handle_register(const protocol::Packet& in, const net::Endpoint& from, std::int64_t now,
                         std::vector<Outgoing>& out);
    void handle_discover(const protocol::Packet& in, const net::Endpoint& from,
                         std::vector<Outgoing>& out);
    void handle_connect(const protocol::Packet& in, std::vector<Outgoing>& out);
    void handle_heartbeat(const protocol::Packet& in, const net::Endpoint& from, std::int64_t now,
                          std::vector<Outgoing>& out);
    void handle_disconnect(const protocol::Packet& in);

    struct Pending {
        net::Bytes challenge;   // == crypto::random_bytes(...)
        net::Endpoint endpoint; // observed source; step 3 must match it
        std::int64_t expiry = 0;
    };

    Config config_;
    PeerRegistry registry_;
    net::UdpSocket socket_;
    std::unordered_map<std::string, Pending> pending_; // key: hex(public key)
    bool running_ = false;
};

} // namespace openmesh::signaling
