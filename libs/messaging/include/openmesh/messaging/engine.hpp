#pragma once

#include "openmesh/messaging/session.hpp"
#include "openmesh/net/endpoint.hpp"
#include "openmesh/net/udp_socket.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

namespace openmesh::messaging {

// A decrypted message as seen by the application layer (SRS FR-4).
struct Message {
    Bytes peer;      // remote identity (Ed25519 public key)
    Bytes plaintext; // decrypted content (only ever in memory / local storage)
    bool outgoing = false;
};

// The UI-independent Messaging Engine (SRS §10). Ties the crypto sessions to the
// UDP transport: it owns a socket, a set of per-peer Sessions, and each peer's
// endpoint, and moves encrypted MESSAGE packets between them. No Qt/UI types.
//
// Peers must be known first (add_peer) — establishing who may message you is the
// contact-request flow (SRS FR-2), out of scope here. Synchronous poll() for v1;
// an event-loop/async receive path is a follow-up.
class Engine {
public:
    using MessageHandler = std::function<void(const Message&)>;

    // `local_public`/`local_secret` are this device's Ed25519 identity.
    Engine(Bytes local_public, Bytes local_secret);

    // Bind the transport socket (host may be "0.0.0.0"/"::"; port 0 = ephemeral).
    bool bind(const net::Endpoint& local);
    [[nodiscard]] std::optional<net::Endpoint> local_endpoint() const;

    // Register a peer we can exchange messages with: its identity and where to
    // reach it. Establishes the crypto session; returns false if key agreement
    // fails or the socket is not bound.
    bool add_peer(const Bytes& remote_public, const net::Endpoint& endpoint);
    [[nodiscard]] bool knows_peer(const Bytes& remote_public) const;

    // Encrypt and send `plaintext` to a known peer. Returns false if the peer is
    // unknown or the datagram could not be sent.
    bool send(const Bytes& remote_public, const Bytes& plaintext);

    // Receive at most one datagram (waiting up to timeout_ms), decrypt it, and
    // invoke the handler. Returns true iff a message was decrypted and delivered.
    bool poll(int timeout_ms);

    void on_message(MessageHandler handler) { handler_ = std::move(handler); }

private:
    Bytes local_public_;
    Bytes local_secret_;
    net::UdpSocket socket_;
    MessageHandler handler_;

    // Keyed by hex(remote public key).
    std::unordered_map<std::string, Session> sessions_;
    std::unordered_map<std::string, net::Endpoint> endpoints_;
};

} // namespace openmesh::messaging
