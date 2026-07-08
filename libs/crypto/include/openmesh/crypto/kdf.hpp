#pragma once

#include "openmesh/crypto/common.hpp"

#include <string>

namespace openmesh::crypto {

// Salt length for derive_key (== crypto_pwhash_SALTBYTES).
inline constexpr std::size_t kKdfSaltBytes = 16;

// Derive a 32-byte symmetric key from a passphrase and salt using Argon2id
// (SRS §9 at-rest key storage). Deliberately slow/memory-hard to resist brute
// force. Returns an empty vector if the salt is the wrong size or derivation
// fails (e.g. out of memory).
[[nodiscard]] Bytes derive_key(const std::string& passphrase, const Bytes& salt);

// A fresh random salt of kKdfSaltBytes for a new derivation.
[[nodiscard]] Bytes generate_salt();

} // namespace openmesh::crypto
