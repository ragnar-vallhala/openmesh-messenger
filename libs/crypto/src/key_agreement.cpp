#include "openmesh/crypto/key_agreement.hpp"

#include "openmesh/crypto/init.hpp"

#include <cstring>
#include <sodium.h>
#include <utility>

namespace openmesh::crypto {

static_assert(kSessionKeyBytes == crypto_generichash_KEYBYTES);

std::optional<Bytes> agree_shared_key(const Bytes& my_secret_key, const Bytes& their_public_key) {
    if (my_secret_key.size() != kSecretKeyBytes || their_public_key.size() != kPublicKeyBytes) {
        return std::nullopt;
    }
    ensure_init();

    // Recover our Ed25519 public key so we can derive our own Curve25519 pk.
    unsigned char my_ed_pk[crypto_sign_PUBLICKEYBYTES];
    crypto_sign_ed25519_sk_to_pk(my_ed_pk, my_secret_key.data());

    // Convert both identities' Ed25519 keys to Curve25519.
    unsigned char my_curve_sk[crypto_scalarmult_SCALARBYTES];
    unsigned char my_curve_pk[crypto_scalarmult_BYTES];
    unsigned char their_curve_pk[crypto_scalarmult_BYTES];
    if (crypto_sign_ed25519_sk_to_curve25519(my_curve_sk, my_secret_key.data()) != 0 ||
        crypto_sign_ed25519_pk_to_curve25519(my_curve_pk, my_ed_pk) != 0 ||
        crypto_sign_ed25519_pk_to_curve25519(their_curve_pk, their_public_key.data()) != 0) {
        sodium_memzero(my_curve_sk, sizeof(my_curve_sk));
        return std::nullopt;
    }

    // Raw Diffie-Hellman shared point. Fails on low-order (degenerate) points.
    unsigned char dh[crypto_scalarmult_BYTES];
    const int rc = crypto_scalarmult(dh, my_curve_sk, their_curve_pk);
    sodium_memzero(my_curve_sk, sizeof(my_curve_sk));
    if (rc != 0) {
        return std::nullopt;
    }

    // Hash dh together with both public keys in a canonical (sorted) order so
    // both peers derive an identical session key regardless of who initiates.
    const unsigned char* lo = my_curve_pk;
    const unsigned char* hi = their_curve_pk;
    if (std::memcmp(lo, hi, crypto_scalarmult_BYTES) > 0) {
        std::swap(lo, hi);
    }

    crypto_generichash_state state;
    crypto_generichash_init(&state, nullptr, 0, kSessionKeyBytes);
    crypto_generichash_update(&state, dh, sizeof(dh));
    crypto_generichash_update(&state, lo, crypto_scalarmult_BYTES);
    crypto_generichash_update(&state, hi, crypto_scalarmult_BYTES);

    Bytes key(kSessionKeyBytes);
    crypto_generichash_final(&state, key.data(), key.size());
    sodium_memzero(dh, sizeof(dh));
    return key;
}

} // namespace openmesh::crypto
