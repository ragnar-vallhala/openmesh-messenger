#include "openmesh/crypto/identity.hpp"

#include <iostream>

// om-keygen: generate a new identity and print its fingerprint (SRS FR-1).
//
// ⚠️ Uses the placeholder crypto backend — output is NOT secure yet.
int main() {
    auto id = openmesh::crypto::generate_identity();
    std::cout << "fingerprint: " << id.fingerprint() << '\n';
    std::cout << "public_key_bytes: " << id.public_key.size() << '\n';
    std::cerr << "warning: placeholder crypto — do not use for real identities\n";
    return 0;
}
