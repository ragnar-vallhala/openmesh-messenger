#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace openmesh::storage {

// Trust state of a contact (SRS FR-2, FR-3).
enum class TrustStatus { Pending, Accepted, Rejected, Blocked };

// A locally stored contact (SRS FR-3).
struct Contact {
    std::vector<std::uint8_t> public_key;
    std::string   nickname;
    TrustStatus   trust     = TrustStatus::Pending;
    std::int64_t  last_seen = 0; // Unix epoch seconds; 0 = never.
    // Session information is attached at runtime by the messaging layer.
};

} // namespace openmesh::storage
