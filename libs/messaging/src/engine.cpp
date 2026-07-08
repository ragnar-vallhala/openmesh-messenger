#include "openmesh/messaging/engine.hpp"

#include "openmesh/core/hex.hpp"
#include "openmesh/protocol/packet.hpp"

#include <utility>

namespace openmesh::messaging {

Engine::Engine(Bytes local_public, Bytes local_secret)
    : local_public_(std::move(local_public)), local_secret_(std::move(local_secret)) {
}

bool Engine::bind(const net::Endpoint& local) {
    return socket_.bind(local);
}

std::optional<net::Endpoint> Engine::local_endpoint() const {
    return socket_.local_endpoint();
}

bool Engine::add_peer(const Bytes& remote_public, const net::Endpoint& endpoint) {
    Session session(local_public_, local_secret_, remote_public);
    if (!session.valid()) {
        return false;
    }
    const std::string key = core::to_hex(remote_public);
    sessions_.insert_or_assign(key, std::move(session));
    endpoints_.insert_or_assign(key, endpoint);
    return true;
}

bool Engine::knows_peer(const Bytes& remote_public) const {
    return sessions_.find(core::to_hex(remote_public)) != sessions_.end();
}

bool Engine::send(const Bytes& remote_public, const Bytes& plaintext) {
    const std::string key = core::to_hex(remote_public);
    auto session = sessions_.find(key);
    auto endpoint = endpoints_.find(key);
    if (session == sessions_.end() || endpoint == endpoints_.end()) {
        return false;
    }
    const protocol::Packet packet = session->second.encrypt(plaintext);
    return socket_.send_to(endpoint->second, protocol::serialize(packet));
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
    const std::string key = core::to_hex(packet.source);
    auto session = sessions_.find(key);
    if (session == sessions_.end()) {
        return false; // message from an unknown peer
    }
    auto plaintext = session->second.decrypt(packet);
    if (!plaintext) {
        return false;
    }
    // Track the peer's current address so replies reach it even if it roamed.
    endpoints_.insert_or_assign(key, datagram->from);
    if (handler_) {
        handler_(Message{packet.source, std::move(*plaintext), false});
    }
    return true;
}

} // namespace openmesh::messaging
