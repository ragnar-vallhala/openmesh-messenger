#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace openmesh::crypto {

using Bytes = std::vector<std::uint8_t>;

// A user's long-term cryptographic identity (SRS §4, FR-1).
// The public key (or its fingerprint) is the user's permanent identity —
// there are no usernames, emails or phone numbers.
struct Identity {
    Bytes public_key;
    Bytes private_key; // MUST be stored securely; never leaves the device.

    // Human-shareable fingerprint of the public key (SRS FR-1).
    [[nodiscard]] std::string fingerprint() const;
};

// Generate a fresh identity key pair.
//
// ⚠️ PLACEHOLDER — the current implementation is NOT cryptographically secure.
// Replace with libsodium (Ed25519/X25519) before any real use.
Identity generate_identity();

} // namespace openmesh::crypto
