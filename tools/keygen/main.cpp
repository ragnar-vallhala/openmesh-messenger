#include "openmesh/crypto/identity.hpp"

#include <iostream>

// om-keygen: generate a new identity and print its fingerprint (SRS FR-1).
// Backed by libsodium (Ed25519).
int main() {
    auto id = openmesh::crypto::generate_identity();
    std::cout << "fingerprint: " << id.fingerprint() << '\n';
    std::cout << "public_key_bytes: " << id.public_key.size() << '\n';
    std::cout << "secret_key_bytes: " << id.secret_key.size() << '\n';
    std::cerr << "note: the secret key is not printed; store it securely\n";
    return 0;
}
