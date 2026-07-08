#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Shared crypto vocabulary. Deliberately free of <sodium.h> so libsodium stays
// an implementation detail (see docs/crypto/design.md). The byte-size constants
// mirror libsodium's Ed25519 / XChaCha20-Poly1305 / BLAKE2b parameters and are
// static_assert-checked against the real macros in the .cpp files.
namespace openmesh::crypto {

using Bytes = std::vector<std::uint8_t>;

inline constexpr std::size_t kPublicKeyBytes = 32;   // Ed25519 public key
inline constexpr std::size_t kSecretKeyBytes = 64;   // Ed25519 secret key (seed+pk)
inline constexpr std::size_t kSignatureBytes = 64;   // Ed25519 detached signature
inline constexpr std::size_t kFingerprintBytes = 32; // BLAKE2b-256 of the public key
inline constexpr std::size_t kSessionKeyBytes = 32;  // XChaCha20-Poly1305 key
inline constexpr std::size_t kAeadNonceBytes = 24;   // XChaCha20-Poly1305 nonce
inline constexpr std::size_t kAeadTagBytes = 16;     // Poly1305 auth tag

} // namespace openmesh::crypto
