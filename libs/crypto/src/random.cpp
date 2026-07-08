#include "openmesh/crypto/random.hpp"

#include "openmesh/crypto/init.hpp"

#include <sodium.h>

namespace openmesh::crypto {

Bytes random_bytes(std::size_t count) {
    ensure_init();
    Bytes out(count);
    if (count > 0) {
        randombytes_buf(out.data(), count);
    }
    return out;
}

} // namespace openmesh::crypto
