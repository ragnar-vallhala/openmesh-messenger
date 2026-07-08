#pragma once

#include "openmesh/crypto/common.hpp"

#include <optional>
#include <string>

namespace openmesh::crypto {

// A user's long-term cryptographic identity (SRS §4, FR-1).
//
// The identity is an Ed25519 key pair. The public key (via its fingerprint) is
// the user's permanent identity — there are no usernames, emails or phone
// numbers. The X25519 keys used for key agreement are derived from this Ed25519
// pair on demand (see key_agreement.hpp), so there is a single identity key.
struct Identity {
    Bytes public_key; // Ed25519 public key (kPublicKeyBytes)
    Bytes secret_key; // Ed25519 secret key (kSecretKeyBytes); never leaves the device

    // Shareable fingerprint: lowercase hex of BLAKE2b-256(public_key) (SRS FR-1).
    [[nodiscard]] std::string fingerprint() const;

    [[nodiscard]] bool valid() const {
        return public_key.size() == kPublicKeyBytes && secret_key.size() == kSecretKeyBytes;
    }
};

// Generate a fresh identity key pair using a CSPRNG.
Identity generate_identity();

// Reconstruct an identity from an exported secret key (import; SRS FR-1).
// Returns nullopt if the secret key is the wrong size.
std::optional<Identity> identity_from_secret(const Bytes& secret_key);

} // namespace openmesh::crypto
