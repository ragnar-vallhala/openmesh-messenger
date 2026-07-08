#pragma once

#include "openmesh/crypto/identity.hpp"

#include <optional>
#include <string>

namespace openmesh::storage {

// At-rest storage of the local identity — the private key kept secure on device
// (SRS §9 secure key storage, FR-1). The key pair is encrypted under a passphrase
// (Argon2id-derived key + XChaCha20-Poly1305).

// Encrypt and write `identity` to `path`. Returns false on I/O/derivation error.
bool save_identity(const std::string& path, const crypto::Identity& identity,
                   const std::string& passphrase);

// Load and decrypt an identity. Returns nullopt on missing file, wrong
// passphrase, or a tampered file.
std::optional<crypto::Identity> load_identity(const std::string& path,
                                              const std::string& passphrase);

} // namespace openmesh::storage
