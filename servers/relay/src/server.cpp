#include "server.hpp"

#include "openmesh/core/hex.hpp"
#include "openmesh/protocol/packet.hpp"

#include <ctime>

namespace openmesh::relay {

RelayServer::RelayServer() : RelayServer(Config{}) {
}

RelayServer::RelayServer(Config config) : config_(config) {
}

void RelayServer::handle(const net::Bytes& datagram, const net::Endpoint& from, std::int64_t now,
                         std::vector<Forward>& out) {
    protocol::Packet packet;
    if (!protocol::deserialize(datagram, packet)) {
        return; // opaque/garbage — not our protocol
    }

    // Learn where this peer is reachable (also refreshes on every packet it sends).
    if (!packet.source.empty()) {
        routes_.upsert(core::to_hex(packet.source), from, now);
    }

    // Forward anything addressed to a peer we know a route to. The payload stays
    // encrypted and is never inspected.
    if (!packet.destination.empty()) {
        if (auto endpoint = routes_.lookup(core::to_hex(packet.destination))) {
            out.push_back({*endpoint, datagram});
        }
    }
    // A packet with no destination is a pure announce/keepalive: recording the
    // source above is all that's needed.
}

void RelayServer::expire(std::int64_t now) {
    routes_.expire(now - config_.route_ttl_seconds);
}

bool RelayServer::bind(const net::Endpoint& local) {
    return socket_.bind(local);
}

bool RelayServer::run_once(int timeout_ms, std::int64_t now) {
    bool processed = false;
    if (auto datagram = socket_.receive(timeout_ms)) {
        std::vector<Forward> out;
        handle(datagram->data, datagram->from, now, out);
        for (const auto& f : out) {
            socket_.send_to(f.to, f.datagram);
        }
        processed = true;
    }
    expire(now);
    return processed;
}

void RelayServer::run() {
    running_ = true;
    while (running_) {
        const auto now = static_cast<std::int64_t>(std::time(nullptr));
        run_once(/*timeout_ms=*/500, now);
    }
}

void RelayServer::stop() {
    running_ = false;
}

std::optional<net::Endpoint> RelayServer::local_endpoint() const {
    return socket_.local_endpoint();
}

} // namespace openmesh::relay
