#pragma once

#include "openmesh/crypto/common.hpp"

#include <cstddef>

namespace openmesh::crypto {

// Cryptographically secure random bytes (libsodium CSPRNG). Used e.g. for the
// signaling proof-of-ownership challenge nonce.
Bytes random_bytes(std::size_t count);

} // namespace openmesh::crypto
