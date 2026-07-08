#include "openmesh/crypto/identity.hpp"

namespace openmesh::crypto {

namespace {
constexpr char kHex[] = "0123456789abcdef";
} // namespace

std::string Identity::fingerprint() const {
    // Placeholder: hex of the public key. The real implementation should hash
    // the public key (e.g. SHA-256/BLAKE2b) and format it in grouped blocks
    // for human verification.
    std::string out;
    out.reserve(public_key.size() * 2);
    for (std::uint8_t b : public_key) {
        out.push_back(kHex[b >> 4]);
        out.push_back(kHex[b & 0x0F]);
    }
    return out;
}

Identity generate_identity() {
    // ⚠️ NOT SECURE. Deterministic placeholder so the build and higher layers
    // can be wired up. Replace with crypto_sign_keypair()/crypto_box_keypair().
    Identity id;
    id.public_key = Bytes(32, 0x00);
    id.private_key = Bytes(32, 0x00);
    return id;
}

} // namespace openmesh::crypto
