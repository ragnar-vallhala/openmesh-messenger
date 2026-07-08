#pragma once

#include "openmesh/crypto/common.hpp"

namespace openmesh::crypto {

// Ed25519 detached signatures. Used for proof of key ownership (SRS §11): a peer
// signs a server-issued challenge so the server can verify the peer holds the
// private key without ever seeing it.

// Sign `message` with an Ed25519 secret key. Returns an empty vector if the
// secret key is the wrong size.
[[nodiscard]] Bytes sign_detached(const Bytes& message, const Bytes& secret_key);

// Verify a detached signature against `message` and an Ed25519 public key.
[[nodiscard]] bool verify_detached(const Bytes& signature, const Bytes& message,
                                   const Bytes& public_key);

} // namespace openmesh::crypto
