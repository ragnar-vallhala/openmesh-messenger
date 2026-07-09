#pragma once

#include "openmesh/net/udp_socket.hpp"
#include "openmesh/protocol/packet.hpp"

#include <functional>
#include <optional>

namespace openmesh::net {

// Client side of the signaling sub-protocol (SRS FR-6; see
// docs/protocol/signaling.md). Talks to a signaling server to register the local
// identity (proof-of-ownership handshake) and to discover peers.
//
// Kept crypto-free: signing the challenge is delegated to an injected `Signer`
// callback, so the caller (which owns the private key via openmesh::crypto)
// supplies it and this library keeps its clean layering.
//
// Operates over a *borrowed* UdpSocket the caller owns. This lets signaling share
// the very socket used for messaging, so the endpoint the server observes and
// registers is exactly where MESSAGE packets will arrive (see Engine::transport()).
// The socket must outlive the client.
//
// Synchronous, blocking-with-timeout for v1. A non-blocking/event-loop variant
// is a follow-up; while awaiting a specific reply, unrelated inbound datagrams
// are currently discarded — so drive signaling before the messaging receive loop
// starts, since the two share one socket.
class SignalingClient {
public:
    // Produces the signature over a challenge nonce (Ed25519, 64 bytes).
    using Signer = std::function<Bytes(const Bytes& challenge)>;

    struct DiscoverResult {
        bool responded = false;           // did the server answer at all?
        std::optional<Endpoint> endpoint; // set iff the peer is online
    };

    static constexpr int kDefaultTimeoutMs = 700;
    static constexpr int kDefaultAttempts = 3;

    // `socket` is the (caller-owned) transport to use; `public_key` is the local
    // Ed25519 identity used as the source id.
    SignalingClient(UdpSocket& socket, Endpoint server, Bytes public_key, Signer signer);

    // Run the REGISTER proof-of-ownership handshake. Retries the whole handshake
    // up to `attempts` times (a fresh challenge each time), tolerating loss of
    // any single datagram. Returns true once the server ACKs.
    bool register_self(int timeout_ms = kDefaultTimeoutMs, int attempts = kDefaultAttempts);

    // Look up a peer by its Ed25519 public key.
    DiscoverResult discover(const Bytes& target_public_key, int timeout_ms = kDefaultTimeoutMs,
                            int attempts = kDefaultAttempts);

    // Relay our connection/ICE info to a peer via the server (fire-and-forget).
    bool send_connect(const Bytes& target_public_key, const Bytes& info);

    // Keepalive; returns true if the server echoes a HEARTBEAT.
    bool heartbeat(int timeout_ms = kDefaultTimeoutMs);

    // Tell the server to forget us (fire-and-forget).
    void disconnect();

    [[nodiscard]] std::optional<Endpoint> local_endpoint() const {
        return socket_.local_endpoint();
    }
    [[nodiscard]] const UdpSocket& socket() const { return socket_; }

    // Our public endpoint as observed by the signaling server (STUN), learned on
    // a successful register_self(). Used to advertise ourselves for direct/hole-
    // punched connections.
    [[nodiscard]] std::optional<Endpoint> public_endpoint() const { return public_endpoint_; }

private:
    bool send(const protocol::Packet& packet);
    // Receive datagrams until one of `expected` type arrives or the timeout
    // elapses; unrelated packets are discarded.
    std::optional<protocol::Packet> await_packet(protocol::PacketType expected, int timeout_ms);

    UdpSocket& socket_; // borrowed; owned by the caller
    Endpoint server_;
    Bytes public_key_;
    Signer signer_;
    std::optional<Endpoint> public_endpoint_;
};

} // namespace openmesh::net
