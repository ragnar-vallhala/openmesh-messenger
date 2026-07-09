#include "openmesh/messaging/engine.hpp"

#include "openmesh/core/hex.hpp"
#include "openmesh/protocol/packet.hpp"

#include <utility>

namespace openmesh::messaging {
namespace {

using protocol::PacketType;

// Contact-response payload: one byte, accept (0x01) or reject (0x00).
constexpr std::uint8_t kAccept = 0x01;
constexpr std::uint8_t kReject = 0x00;

storage::Contact make_contact(const Bytes& public_key, storage::TrustStatus trust) {
    storage::Contact c;
    c.public_key = public_key;
    c.trust = trust;
    return c;
}

} // namespace

Engine::Engine(Bytes local_public, Bytes local_secret)
    : local_public_(std::move(local_public)), local_secret_(std::move(local_secret)) {
}

bool Engine::bind(const net::Endpoint& local) {
    return socket_.bind(local);
}

std::optional<net::Endpoint> Engine::local_endpoint() const {
    return socket_.local_endpoint();
}

bool Engine::announce_to(const net::Endpoint& relay) {
    // A source-tagged, no-destination packet: the relay records where to reach us
    // but forwards nothing.
    protocol::Packet hello;
    hello.type = protocol::PacketType::Hello;
    hello.source = local_public_;
    return socket_.send_to(relay, protocol::serialize(hello));
}

Session* Engine::ensure_session(const Bytes& remote_public) {
    const std::string key = core::to_hex(remote_public);
    auto it = sessions_.find(key);
    if (it != sessions_.end()) {
        return &it->second;
    }
    Session session(local_public_, local_secret_, remote_public);
    if (!session.valid()) {
        return nullptr;
    }
    auto [inserted, _] = sessions_.insert_or_assign(key, std::move(session));
    return &inserted->second;
}

bool Engine::send_packet(const std::string& key, const protocol::Packet& packet) {
    auto endpoint = endpoints_.find(key);
    if (endpoint == endpoints_.end()) {
        return false;
    }
    return socket_.send_to(endpoint->second, protocol::serialize(packet));
}

bool Engine::add_peer(const Bytes& remote_public, const net::Endpoint& endpoint) {
    if (!ensure_session(remote_public)) {
        return false;
    }
    const std::string key = core::to_hex(remote_public);
    endpoints_.insert_or_assign(key, endpoint);
    contacts_.upsert(make_contact(remote_public, storage::TrustStatus::Accepted));
    return true;
}

bool Engine::knows_peer(const Bytes& remote_public) const {
    return sessions_.find(core::to_hex(remote_public)) != sessions_.end();
}

std::optional<storage::TrustStatus> Engine::trust_of(const Bytes& remote_public) const {
    return contacts_.trust_of(remote_public);
}

bool Engine::send_contact_request(const Bytes& remote_public, const net::Endpoint& endpoint,
                                  const Bytes& greeting) {
    if (contacts_.trust_of(remote_public) == storage::TrustStatus::Accepted) {
        return false; // already a contact
    }
    Session* session = ensure_session(remote_public);
    if (!session) {
        return false;
    }
    const std::string key = core::to_hex(remote_public);
    endpoints_.insert_or_assign(key, endpoint);
    outgoing_requests_.insert(key);
    contacts_.upsert(make_contact(remote_public, storage::TrustStatus::Pending));
    return send_packet(key, session->encrypt_as(PacketType::ContactRequest, greeting));
}

bool Engine::accept_contact(const Bytes& remote_public) {
    const std::string key = core::to_hex(remote_public);
    if (incoming_requests_.find(key) == incoming_requests_.end()) {
        return false; // no pending incoming request from this peer
    }
    Session* session = ensure_session(remote_public);
    if (!session) {
        return false;
    }
    incoming_requests_.erase(key);
    contacts_.upsert(make_contact(remote_public, storage::TrustStatus::Accepted));
    return send_packet(key, session->encrypt_as(PacketType::ContactResponse, Bytes{kAccept}));
}

bool Engine::reject_contact(const Bytes& remote_public) {
    const std::string key = core::to_hex(remote_public);
    if (incoming_requests_.find(key) == incoming_requests_.end()) {
        return false;
    }
    Session* session = ensure_session(remote_public);
    if (!session) {
        return false;
    }
    incoming_requests_.erase(key);
    contacts_.upsert(make_contact(remote_public, storage::TrustStatus::Rejected));
    return send_packet(key, session->encrypt_as(PacketType::ContactResponse, Bytes{kReject}));
}

bool Engine::send(const Bytes& remote_public, const Bytes& plaintext) {
    if (contacts_.trust_of(remote_public) != storage::TrustStatus::Accepted) {
        return false; // only accepted contacts can exchange messages (SRS FR-2, §7)
    }
    Session* session = ensure_session(remote_public);
    if (!session) {
        return false;
    }
    return send_packet(core::to_hex(remote_public), session->encrypt(plaintext));
}

bool Engine::poll(int timeout_ms) {
    auto datagram = socket_.receive(timeout_ms);
    if (!datagram) {
        return false;
    }
    protocol::Packet packet;
    if (!protocol::deserialize(datagram->data, packet)) {
        return false;
    }
    const Bytes& peer = packet.source;

    // Spam gate: drop a message from a non-accepted peer before any crypto work.
    if (packet.type == PacketType::Message &&
        contacts_.trust_of(peer) != storage::TrustStatus::Accepted) {
        return false;
    }

    Session* session = ensure_session(peer);
    if (!session) {
        return false;
    }
    auto plaintext = session->decrypt(packet);
    if (!plaintext) {
        return false;
    }
    // Track the peer's current address so replies reach it.
    endpoints_.insert_or_assign(core::to_hex(peer), datagram->from);

    switch (packet.type) {
    case PacketType::Message:
        handle_message(peer, std::move(*plaintext));
        return true;
    case PacketType::ContactRequest:
        handle_contact_request(peer, std::move(*plaintext), datagram->from);
        return true;
    case PacketType::ContactResponse:
        handle_contact_response(peer, *plaintext);
        return true;
    default:
        return false;
    }
}

void Engine::handle_message(const Bytes& peer, Bytes plaintext) {
    if (message_handler_) {
        message_handler_(Message{peer, std::move(plaintext), false});
    }
}

void Engine::handle_contact_request(const Bytes& peer, Bytes greeting, const net::Endpoint&) {
    const std::string key = core::to_hex(peer);
    const auto trust = contacts_.trust_of(peer);
    // One-request rule (SRS §7): surface only the first request from an otherwise
    // unknown peer. Already-known peers (pending/accepted/rejected/blocked) are
    // silently ignored.
    if (trust.has_value() || incoming_requests_.count(key) != 0) {
        return;
    }
    incoming_requests_.insert(key);
    contacts_.upsert(make_contact(peer, storage::TrustStatus::Pending));
    if (request_handler_) {
        request_handler_(peer, greeting);
    }
}

void Engine::handle_contact_response(const Bytes& peer, const Bytes& decision) {
    const std::string key = core::to_hex(peer);
    if (outgoing_requests_.find(key) == outgoing_requests_.end()) {
        return; // unsolicited response
    }
    outgoing_requests_.erase(key);
    const bool accepted = !decision.empty() && decision[0] == kAccept;
    contacts_.upsert(make_contact(peer, accepted ? storage::TrustStatus::Accepted
                                                 : storage::TrustStatus::Rejected));
    if (response_handler_) {
        response_handler_(peer, accepted);
    }
}

} // namespace openmesh::messaging
