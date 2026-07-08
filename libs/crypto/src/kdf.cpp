#include "openmesh/crypto/kdf.hpp"

#include "openmesh/crypto/init.hpp"
#include "openmesh/crypto/random.hpp"

#include <sodium.h>

namespace openmesh::crypto {

static_assert(kKdfSaltBytes == crypto_pwhash_SALTBYTES);
static_assert(kSessionKeyBytes == 32);

Bytes derive_key(const std::string& passphrase, const Bytes& salt) {
    if (salt.size() != kKdfSaltBytes) {
        return {};
    }
    ensure_init();
    Bytes key(kSessionKeyBytes);
    if (crypto_pwhash(key.data(), key.size(), passphrase.c_str(), passphrase.size(), salt.data(),
                      crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_ARGON2ID13) != 0) {
        return {}; // out of memory
    }
    return key;
}

Bytes generate_salt() {
    return random_bytes(kKdfSaltBytes);
}

} // namespace openmesh::crypto
