#include "openmesh/crypto/init.hpp"

#include <sodium.h>

namespace openmesh::crypto {

bool ensure_init() {
    // sodium_init() is idempotent and thread-safe: 0 = initialized now,
    // 1 = already initialized, -1 = failure. Run once via a function-local static.
    static const int result = sodium_init();
    return result >= 0;
}

} // namespace openmesh::crypto
