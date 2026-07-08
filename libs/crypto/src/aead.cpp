#include "openmesh/crypto/aead.hpp"

#include "openmesh/crypto/init.hpp"

#include <sodium.h>

namespace openmesh::crypto {

static_assert(kSessionKeyBytes == crypto_aead_xchacha20poly1305_ietf_KEYBYTES);
static_assert(kAeadNonceBytes == crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
static_assert(kAeadTagBytes == crypto_aead_xchacha20poly1305_ietf_ABYTES);

Bytes seal(const Bytes& key, const Bytes& plaintext, const Bytes& aad) {
    if (key.size() != kSessionKeyBytes) {
        return {};
    }
    ensure_init();

    // Layout: nonce || ciphertext || tag.
    Bytes out(kAeadNonceBytes + plaintext.size() + kAeadTagBytes);
    randombytes_buf(out.data(), kAeadNonceBytes);

    unsigned long long cipher_len = 0;
    crypto_aead_xchacha20poly1305_ietf_encrypt(
        out.data() + kAeadNonceBytes, &cipher_len, plaintext.data(), plaintext.size(), aad.data(),
        aad.size(), nullptr, out.data() /* nonce */, key.data());

    out.resize(kAeadNonceBytes + static_cast<std::size_t>(cipher_len));
    return out;
}

std::optional<Bytes> open(const Bytes& key, const Bytes& sealed, const Bytes& aad) {
    if (key.size() != kSessionKeyBytes || sealed.size() < kAeadNonceBytes + kAeadTagBytes) {
        return std::nullopt;
    }
    ensure_init();

    const unsigned char* nonce = sealed.data();
    const unsigned char* cipher = sealed.data() + kAeadNonceBytes;
    const std::size_t cipher_len = sealed.size() - kAeadNonceBytes;

    Bytes plaintext(cipher_len - kAeadTagBytes);
    unsigned long long plain_len = 0;
    if (crypto_aead_xchacha20poly1305_ietf_decrypt(plaintext.data(), &plain_len, nullptr, cipher,
                                                   cipher_len, aad.data(), aad.size(), nonce,
                                                   key.data()) != 0) {
        return std::nullopt; // authentication failed: tampered ciphertext or aad
    }

    plaintext.resize(static_cast<std::size_t>(plain_len));
    return plaintext;
}

} // namespace openmesh::crypto
