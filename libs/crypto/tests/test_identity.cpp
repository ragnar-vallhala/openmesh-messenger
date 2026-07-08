#include "openmesh/crypto/identity.hpp"

#include <cassert>

int main() {
    using namespace openmesh::crypto;

    Identity id = generate_identity();
    assert(!id.public_key.empty());
    assert(!id.private_key.empty());

    // Fingerprint is hex of the public key -> two chars per byte.
    assert(id.fingerprint().size() == id.public_key.size() * 2);
    return 0;
}
