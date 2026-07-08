#include "server.hpp"

#include "openmesh/core/hex.hpp"
#include "openmesh/core/logging.hpp"
#include "openmesh/crypto/common.hpp"
#include "openmesh/crypto/random.hpp"
#include "openmesh/crypto/sign.hpp"

#include <ctime>
#include <string>

namespace openmesh::signaling {
namespace {

using net::Bytes;
using net::Endpoint;
using protocol::Packet;
using protocol::PacketType;

constexpr std::size_t kChallengeBytes = 32;

using core::to_hex;

Bytes to_bytes(const std::string& s) {
    return Bytes(s.begin(), s.end());
}

Packet make(PacketType type, Bytes source = {}, Bytes destination = {}, Bytes payload = {}) {
    Packet p;
    p.type = type;
    p.source = std::move(source);
    p.destination = std::move(destination);
    p.payload = std::move(payload);
    return p;
}

} // namespace

SignalingServer::SignalingServer() : SignalingServer(Config{}) {
}

SignalingServer::SignalingServer(Config config) : config_(config) {
}

void SignalingServer::handle(const Packet& in, const Endpoint& from, std::int64_t now,
                             std::vector<Outgoing>& out) {
    switch (in.type) {
    case PacketType::Register:
        handle_register(in, from, now, out);
        break;
    case PacketType::Discover:
        handle_discover(in, from, out);
        break;
    case PacketType::Connect:
        handle_connect(in, out);
        break;
    case PacketType::Heartbeat:
        handle_heartbeat(in, from, now, out);
        break;
    case PacketType::Disconnect:
        handle_disconnect(in);
        break;
    default:
        // HELLO / MESSAGE / ACK / CONTACT_* are not signaling-server business.
        break;
    }
}

void SignalingServer::handle_register(const Packet& in, const Endpoint& from, std::int64_t now,
                                      std::vector<Outgoing>& out) {
    if (in.source.size() != crypto::kPublicKeyBytes) {
        return; // no identity to register
    }
    const std::string key = to_hex(in.source);

    if (in.payload.empty()) {
        // Step 1: issue a fresh challenge bound to the observed source endpoint.
        Bytes challenge = crypto::random_bytes(kChallengeBytes);
        pending_[key] = Pending{challenge, from, now + config_.challenge_ttl_seconds};
        out.push_back({from, make(PacketType::Hello, {}, {}, challenge)});
        return;
    }

    if (in.payload.size() == crypto::kSignatureBytes) {
        // Step 2: verify the signature over the pending challenge.
        auto it = pending_.find(key);
        if (it == pending_.end() || it->second.expiry < now) {
            return; // no/expired challenge
        }
        // Return-routability: the reply must come from the challenged endpoint.
        if (it->second.endpoint.host != from.host || it->second.endpoint.port != from.port) {
            return;
        }
        if (!crypto::verify_detached(in.payload, it->second.challenge, in.source)) {
            core::log_warn("signaling: REGISTER signature verification failed");
            pending_.erase(it);
            return;
        }
        registry_.upsert(key, from, now);
        pending_.erase(it);
        core::log_info("signaling: registered " + key.substr(0, 16) + "… at " + from.to_string());
        out.push_back({from, make(PacketType::Ack)});
    }
}

void SignalingServer::handle_discover(const Packet& in, const Endpoint& from,
                                      std::vector<Outgoing>& out) {
    if (in.destination.empty()) {
        return;
    }
    const std::string key = to_hex(in.destination);
    auto endpoint = registry_.lookup(key);
    // Reply with CONNECT: src = the looked-up identity, payload = "host:port"
    // when online, or empty when offline/unknown.
    Bytes payload = endpoint ? to_bytes(endpoint->to_string()) : Bytes{};
    out.push_back({from, make(PacketType::Connect, in.destination, {}, std::move(payload))});
}

void SignalingServer::handle_connect(const Packet& in, std::vector<Outgoing>& out) {
    // Relay a peer's connection/ICE info to the destination peer, if registered.
    if (in.destination.empty()) {
        return;
    }
    auto endpoint = registry_.lookup(to_hex(in.destination));
    if (endpoint) {
        out.push_back({*endpoint, in}); // forward verbatim (server sees only the envelope)
    }
}

void SignalingServer::handle_heartbeat(const Packet& in, const Endpoint& from, std::int64_t now,
                                       std::vector<Outgoing>& out) {
    if (in.source.size() == crypto::kPublicKeyBytes) {
        const std::string key = to_hex(in.source);
        if (registry_.lookup(key)) {
            registry_.upsert(key, from, now); // refresh liveness
        }
    }
    out.push_back({from, make(PacketType::Heartbeat)});
}

void SignalingServer::handle_disconnect(const Packet& in) {
    if (in.source.size() == crypto::kPublicKeyBytes) {
        registry_.remove(to_hex(in.source));
    }
}

void SignalingServer::expire(std::int64_t now) {
    registry_.expire(now - config_.peer_ttl_seconds);
    for (auto it = pending_.begin(); it != pending_.end();) {
        if (it->second.expiry < now) {
            it = pending_.erase(it);
        } else {
            ++it;
        }
    }
}

bool SignalingServer::bind(const Endpoint& local) {
    return socket_.bind(local);
}

bool SignalingServer::run_once(int timeout_ms, std::int64_t now) {
    bool processed = false;
    if (auto dg = socket_.receive(timeout_ms)) {
        Packet in;
        if (protocol::deserialize(dg->data, in)) {
            std::vector<Outgoing> out;
            handle(in, dg->from, now, out);
            for (const auto& o : out) {
                socket_.send_to(o.to, protocol::serialize(o.packet));
            }
            processed = true;
        }
    }
    expire(now);
    return processed;
}

void SignalingServer::run() {
    running_ = true;
    while (running_) {
        const auto now = static_cast<std::int64_t>(std::time(nullptr));
        run_once(/*timeout_ms=*/500, now);
    }
}

void SignalingServer::stop() {
    running_ = false;
}

std::optional<Endpoint> SignalingServer::local_endpoint() const {
    return socket_.local_endpoint();
}

} // namespace openmesh::signaling
