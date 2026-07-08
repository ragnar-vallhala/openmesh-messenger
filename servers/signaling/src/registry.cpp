#include "registry.hpp"

namespace openmesh::signaling {

void PeerRegistry::upsert(const std::string& fingerprint, const net::Endpoint& endpoint,
                          std::int64_t now_seconds) {
    peers_[fingerprint] = Entry{endpoint, now_seconds};
}

std::optional<net::Endpoint> PeerRegistry::lookup(const std::string& fingerprint) const {
    auto it = peers_.find(fingerprint);
    if (it == peers_.end()) {
        return std::nullopt;
    }
    return it->second.endpoint;
}

void PeerRegistry::expire(std::int64_t cutoff_seconds) {
    for (auto it = peers_.begin(); it != peers_.end();) {
        if (it->second.last_seen < cutoff_seconds) {
            it = peers_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace openmesh::signaling
