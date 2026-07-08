#pragma once

#include "openmesh/net/endpoint.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace openmesh::signaling {

// In-memory registry of active peers (SRS §11). Keyed by the peer's public-key
// fingerprint. Entries expire after inactivity — expiry sweeping is TODO.
class PeerRegistry {
public:
    void upsert(const std::string& fingerprint, const net::Endpoint& endpoint,
                std::int64_t now_seconds);

    [[nodiscard]] std::optional<net::Endpoint> lookup(const std::string& fingerprint) const;

    // Remove a specific peer (e.g. on DISCONNECT). Returns true if one was removed.
    bool remove(const std::string& fingerprint);

    // Remove peers not seen since `cutoff_seconds`.
    void expire(std::int64_t cutoff_seconds);

    [[nodiscard]] std::size_t size() const { return peers_.size(); }

private:
    struct Entry {
        net::Endpoint endpoint;
        std::int64_t  last_seen = 0;
    };
    std::unordered_map<std::string, Entry> peers_;
};

} // namespace openmesh::signaling
