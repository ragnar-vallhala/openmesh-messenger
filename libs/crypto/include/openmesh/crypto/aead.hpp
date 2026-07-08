#pragma once

#include "openmesh/crypto/common.hpp"

#include <optional>

namespace openmesh::crypto {

// Authenticated encryption with associated data (XChaCha20-Poly1305-IETF).
//
// `seal` prepends a fresh random 24-byte nonce to the ciphertext+tag, so the
// returned blob is  nonce || ciphertext || tag  and is self-contained. The
// `aad` (associated data) is authenticated but not encrypted: the caller passes
// the serialized packet envelope (header + routing ids) so any tampering with it
// makes `open` fail (SRS §5; see docs/protocol/wire-format.md §8).

// Encrypt `plaintext`. Returns nonce||ciphertext||tag, or an empty vector if the
// key size is wrong.
[[nodiscard]] Bytes seal(const Bytes& key, const Bytes& plaintext, const Bytes& aad);

// Decrypt a blob produced by `seal`. Returns nullopt if the key is wrong, the
// input is too short, or authentication fails (tampered ciphertext or aad).
[[nodiscard]] std::optional<Bytes> open(const Bytes& key, const Bytes& sealed, const Bytes& aad);

} // namespace openmesh::crypto
