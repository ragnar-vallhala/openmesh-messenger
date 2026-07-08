#include "openmesh/crypto/aead.hpp"
#include "openmesh/crypto/identity.hpp"
#include "openmesh/crypto/key_agreement.hpp"
#include "openmesh/crypto/sign.hpp"

#include <cassert>

using namespace openmesh::crypto;

static void test_identity() {
    Identity id = generate_identity();
    assert(id.valid());
    assert(id.public_key.size() == kPublicKeyBytes);
    assert(id.secret_key.size() == kSecretKeyBytes);

    // Two identities must differ (CSPRNG, not the old zero placeholder).
    Identity other = generate_identity();
    assert(id.public_key != other.public_key);

    // Fingerprint is deterministic hex of a 32-byte BLAKE2b hash.
    assert(id.fingerprint().size() == kFingerprintBytes * 2);
    assert(id.fingerprint() == id.fingerprint());
    assert(id.fingerprint() != other.fingerprint());

    // Export/import round-trips.
    auto reimported = identity_from_secret(id.secret_key);
    assert(reimported.has_value());
    assert(reimported->public_key == id.public_key);
    assert(!identity_from_secret(Bytes{0x00}).has_value());
}

static void test_sign_verify() {
    Identity id = generate_identity();
    const Bytes challenge = {'n', 'o', 'n', 'c', 'e', '1'};

    Bytes sig = sign_detached(challenge, id.secret_key);
    assert(sig.size() == kSignatureBytes);
    assert(verify_detached(sig, challenge, id.public_key));

    // Wrong message, tampered signature, and wrong key all fail.
    assert(!verify_detached(sig, Bytes{'n', 'o', 'n', 'c', 'e', '2'}, id.public_key));
    Bytes bad = sig;
    bad[0] ^= 0xFF;
    assert(!verify_detached(bad, challenge, id.public_key));
    assert(!verify_detached(sig, challenge, generate_identity().public_key));
}

static void test_key_agreement() {
    Identity alice = generate_identity();
    Identity bob = generate_identity();

    auto k_ab = agree_shared_key(alice.secret_key, bob.public_key);
    auto k_ba = agree_shared_key(bob.secret_key, alice.public_key);
    assert(k_ab.has_value() && k_ba.has_value());
    // Both sides derive the identical session key.
    assert(*k_ab == *k_ba);
    assert(k_ab->size() == kSessionKeyBytes);

    // A third party derives a different key.
    Identity eve = generate_identity();
    auto k_ae = agree_shared_key(alice.secret_key, eve.public_key);
    assert(k_ae.has_value() && *k_ae != *k_ab);
}

static void test_aead() {
    Identity alice = generate_identity();
    Identity bob = generate_identity();
    const Bytes key = *agree_shared_key(alice.secret_key, bob.public_key);

    const Bytes plaintext = {'h', 'e', 'l', 'l', 'o'};
    const Bytes aad = {0x4F, 0x4D, 0x01}; // stand-in for the packet envelope

    Bytes sealed = seal(key, plaintext, aad);
    assert(sealed.size() == kAeadNonceBytes + plaintext.size() + kAeadTagBytes);

    // Correct key + aad decrypts.
    auto opened = open(key, sealed, aad);
    assert(opened.has_value() && *opened == plaintext);

    // The receiving side reconstructs the same key and decrypts too.
    const Bytes bob_key = *agree_shared_key(bob.secret_key, alice.public_key);
    assert(open(bob_key, sealed, aad)->size() == plaintext.size());

    // Tampered ciphertext, tampered aad, and wrong key all fail authentication.
    Bytes tampered = sealed;
    tampered.back() ^= 0xFF;
    assert(!open(key, tampered, aad).has_value());
    assert(!open(key, sealed, Bytes{0x00}).has_value());
    assert(!open(*agree_shared_key(alice.secret_key, generate_identity().public_key), sealed, aad)
                .has_value());
}

int main() {
    test_identity();
    test_sign_verify();
    test_key_agreement();
    test_aead();
    return 0;
}
