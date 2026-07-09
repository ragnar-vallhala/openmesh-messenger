#include "router.hpp"

namespace openmesh::relay {

void RouteTable::upsert(const std::string& key, const net::Endpoint& endpoint,
                        std::int64_t now_seconds) {
    routes_[key] = Entry{endpoint, now_seconds};
}

std::optional<net::Endpoint> RouteTable::lookup(const std::string& key) const {
    auto it = routes_.find(key);
    if (it == routes_.end()) {
        return std::nullopt;
    }
    return it->second.endpoint;
}

void RouteTable::expire(std::int64_t cutoff_seconds) {
    for (auto it = routes_.begin(); it != routes_.end();) {
        if (it->second.last_seen < cutoff_seconds) {
            it = routes_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace openmesh::relay
