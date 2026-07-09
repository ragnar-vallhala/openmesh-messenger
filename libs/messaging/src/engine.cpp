#include "openmesh/messaging/engine.hpp"

#include "openmesh/core/hex.hpp"
#include "openmesh/protocol/packet.hpp"

#include <cstdlib>
#include <string>
#include <utility>

namespace openmesh::messaging {
namespace {

using protocol::PacketType;

// Contact-response payload: one byte, accept (0x01) or reject (0x00).
constexpr std::uint8_t kAccept = 0x01;
constexpr std::uint8_t kReject = 0x00;

// Hole-punch tuning: probe up to ~3s, then rely on the relay fallback.
constexpr int kMaxProbes = 12;
constexpr auto kProbeInterval = std::chrono::milliseconds(250);

storage::Contact make_contact(const Bytes& public_key, storage::TrustStatus trust) {
    storage::Contact c;
    c.public_key = public_key;
    c.trust = trust;
    return c;
}

std::optional<net::Endpoint> parse_endpoint(const Bytes& payload) {
    if (payload.empty()) {
        return std::nullopt;
    }
    const std::string s(payload.begin(), payload.end());
    const auto pos = s.rfind(':');
    if (pos == std::string::npos || pos + 1 >= s.size()) {
        return std::nullopt;
    }
    net::Endpoint ep;
    ep.host = s.substr(0, pos);
    ep.port = static_cast<std::uint16_t>(std::strtoul(s.c_str() + pos + 1, nullptr, 10));
    return ep;
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

void Engine::send_probe(const Bytes& peer, const net::Endpoint& to) {
    // A hole-punch probe: HELLO addressed to the peer (a destination distinguishes
    // it from a relay announce). Sending it opens our NAT toward `to`.
    protocol::Packet probe;
    probe.type = protocol::PacketType::Hello;
    probe.source = local_public_;
    probe.destination = peer;
    socket_.send_to(to, protocol::serialize(probe));
}

void Engine::start_probing(const Bytes& peer, const net::Endpoint& candidate) {
    const std::string key = core::to_hex(peer);
    auto it = probes_.find(key);
    if (it != probes_.end() && it->second.confirmed) {
        return; // already have a direct path
    }
    Probe& p = probes_[key];
    p.peer = peer;
    p.candidate = candidate;
    p.last = std::chrono::steady_clock::now();
    p.attempts = 1;
    send_probe(peer, candidate);
}

net::Endpoint Engine::connect(const Bytes& remote_public, const net::Endpoint& candidate) {
    const std::string key = core::to_hex(remote_public);
    // Send via the relay immediately (works through NAT) while we try to punch a
    // direct path; if no relay, use the candidate directly (best effort).
    const net::Endpoint active = relay_ ? *relay_ : candidate;
    endpoints_.insert_or_assign(key, active);
    start_probing(remote_public, candidate);
    return active;
}

void Engine::tick() {
    const auto now = std::chrono::steady_clock::now();
    for (auto& [key, p] : probes_) {
        if (p.confirmed || p.attempts >= kMaxProbes) {
            continue;
        }
        if (now - p.last >= kProbeInterval) {
            send_probe(p.peer, p.candidate);
            ++p.attempts;
            p.last = now;
        }
    }
    // Fallback needs no explicit timer: while a path is unconfirmed the active
    // endpoint is already the relay (set in connect()); a successful probe just
    // upgrades it to direct in handle_probe().
}

bool Engine::has_direct_path(const Bytes& remote_public) const {
    auto it = probes_.find(core::to_hex(remote_public));
    return it != probes_.end() && it->second.confirmed;
}

void Engine::handle_probe(const Bytes& peer, const net::Endpoint& from) {
    const std::string key = core::to_hex(peer);
    Probe& p = probes_[key];
    p.peer = peer;
    const bool was_confirmed = p.confirmed;
    p.confirmed = true;
    // Upgrade to the confirmed direct path.
    endpoints_.insert_or_assign(key, from);
    if (!was_confirmed) {
        // Reply so the peer confirms its side too (once; avoids ping-pong).
        send_probe(peer, from);
    }
}

void Engine::handle_connect(const Bytes& peer, const Bytes& payload) {
    // The peer told us (via signaling) its public endpoint — probe it to punch a
    // hole back. Ensure we at least have the relay as a fallback path meanwhile.
    auto ep = parse_endpoint(payload);
    if (!ep) {
        return;
    }
    const std::string key = core::to_hex(peer);
    if (endpoints_.find(key) == endpoints_.end() && relay_) {
        endpoints_.insert_or_assign(key, *relay_);
    }
    start_probing(peer, *ep);
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

    // Connectivity-control packets (unencrypted, handled before the crypto path).
    if (packet.type == PacketType::Hello) {
        // A hole-punch probe addressed to us.
        if (packet.destination == local_public_ && !peer.empty()) {
            handle_probe(peer, datagram->from);
        }
        return true;
    }
    if (packet.type == PacketType::Connect) {
        handle_connect(peer, packet.payload); // peer advertised its endpoint
        return true;
    }

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
    // Track the peer's current address so replies reach it — but never downgrade a
    // confirmed direct path (e.g. a message that arrived via the relay).
    const std::string key = core::to_hex(peer);
    auto probe = probes_.find(key);
    if (probe == probes_.end() || !probe->second.confirmed) {
        endpoints_.insert_or_assign(key, datagram->from);
    }

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
