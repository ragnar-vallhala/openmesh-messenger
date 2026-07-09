#pragma once

#include "openmesh/net/endpoint.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace openmesh::relay {

// Temporary routing table for the relay (SRS §12): maps a peer's public-key
// fingerprint to the endpoint its packets were last seen from. Entries expire
// after inactivity; peers keep them fresh by periodically announcing.
class RouteTable {
public:
    void upsert(const std::string& key, const net::Endpoint& endpoint, std::int64_t now_seconds);
    [[nodiscard]] std::optional<net::Endpoint> lookup(const std::string& key) const;
    void expire(std::int64_t cutoff_seconds);
    [[nodiscard]] std::size_t size() const { return routes_.size(); }

private:
    struct Entry {
        net::Endpoint endpoint;
        std::int64_t last_seen = 0;
    };
    std::unordered_map<std::string, Entry> routes_;
};

} // namespace openmesh::relay
