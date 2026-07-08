#include "openmesh/crypto/sign.hpp"

#include "openmesh/crypto/init.hpp"

#include <sodium.h>

namespace openmesh::crypto {

static_assert(kSignatureBytes == crypto_sign_BYTES);

Bytes sign_detached(const Bytes& message, const Bytes& secret_key) {
    if (secret_key.size() != kSecretKeyBytes) {
        return {};
    }
    ensure_init();
    Bytes signature(kSignatureBytes);
    crypto_sign_detached(signature.data(), nullptr, message.data(), message.size(),
                         secret_key.data());
    return signature;
}

bool verify_detached(const Bytes& signature, const Bytes& message, const Bytes& public_key) {
    if (signature.size() != kSignatureBytes || public_key.size() != kPublicKeyBytes) {
        return false;
    }
    ensure_init();
    return crypto_sign_verify_detached(signature.data(), message.data(), message.size(),
                                       public_key.data()) == 0;
}

} // namespace openmesh::crypto
