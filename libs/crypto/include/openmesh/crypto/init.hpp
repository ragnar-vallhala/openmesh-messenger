#pragma once

namespace openmesh::crypto {

// Initialize the crypto backend. Idempotent and thread-safe; every public entry
// point in this library calls it, so callers normally never need to. Returns
// false only if libsodium failed to initialize (should not happen in practice).
bool ensure_init();

} // namespace openmesh::crypto
