#include "openmesh/crypto/identity.hpp"

#include "openmesh/crypto/init.hpp"

#include <sodium.h>

namespace openmesh::crypto {

static_assert(kPublicKeyBytes == crypto_sign_PUBLICKEYBYTES);
static_assert(kSecretKeyBytes == crypto_sign_SECRETKEYBYTES);
static_assert(kFingerprintBytes == crypto_generichash_BYTES);

namespace {
constexpr char kHex[] = "0123456789abcdef";
} // namespace

std::string Identity::fingerprint() const {
    ensure_init();
    unsigned char hash[kFingerprintBytes];
    crypto_generichash(hash, sizeof(hash), public_key.data(), public_key.size(), nullptr, 0);

    std::string out;
    out.reserve(sizeof(hash) * 2);
    for (unsigned char b : hash) {
        out.push_back(kHex[b >> 4]);
        out.push_back(kHex[b & 0x0F]);
    }
    return out;
}

Identity generate_identity() {
    ensure_init();
    Identity id;
    id.public_key.resize(kPublicKeyBytes);
    id.secret_key.resize(kSecretKeyBytes);
    crypto_sign_keypair(id.public_key.data(), id.secret_key.data());
    return id;
}

std::optional<Identity> identity_from_secret(const Bytes& secret_key) {
    if (secret_key.size() != kSecretKeyBytes) {
        return std::nullopt;
    }
    ensure_init();
    Identity id;
    id.secret_key = secret_key;
    id.public_key.resize(kPublicKeyBytes);
    // Recover the Ed25519 public key from the secret key.
    crypto_sign_ed25519_sk_to_pk(id.public_key.data(), secret_key.data());
    return id;
}

} // namespace openmesh::crypto
