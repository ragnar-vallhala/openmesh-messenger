#pragma once

#include "openmesh/crypto/common.hpp"

#include <optional>

namespace openmesh::crypto {

// Diffie-Hellman key agreement between two identities.
//
// The Ed25519 identity keys are converted to X25519 and combined with
// crypto_scalarmult; the raw shared point is hashed together with both parties'
// public keys (in a canonical order) so that both peers derive the *same*
// 32-byte session key. This is the building block for session setup; the full
// forward-secret handshake and Double Ratchet are specified in
// docs/crypto/design.md and layered on top.
//
// `my_secret_key` is our Ed25519 secret key (kSecretKeyBytes); `their_public_key`
// is the peer's Ed25519 public key (kPublicKeyBytes). Returns nullopt on bad
// sizes or a degenerate (low-order) shared point.
[[nodiscard]] std::optional<Bytes> agree_shared_key(const Bytes& my_secret_key,
                                                    const Bytes& their_public_key);

} // namespace openmesh::crypto
