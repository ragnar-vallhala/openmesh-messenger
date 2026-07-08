#include "openmesh/core/hex.hpp"
#include "openmesh/crypto/identity.hpp"
#include "openmesh/messaging/engine.hpp"
#include "openmesh/messaging/session.hpp"
#include "openmesh/net/endpoint.hpp"
#include "openmesh/protocol/packet.hpp"

#include <iostream>
#include <string>

using openmesh::crypto::generate_identity;
using openmesh::crypto::Identity;
using openmesh::messaging::Bytes;
using openmesh::messaging::Engine;
using openmesh::messaging::Message;
using openmesh::messaging::Session;
using openmesh::net::Endpoint;

namespace {
Bytes to_bytes(const std::string& s) {
    return Bytes(s.begin(), s.end());
}
std::string to_string(const Bytes& b) {
    return std::string(b.begin(), b.end());
}
std::string short_fp(const Identity& id) {
    return id.fingerprint().substr(0, 16) + "…";
}
} // namespace

// End-to-end demonstration of the encrypted message layer: what the sender puts
// in, what a relay sees on the wire, what the recipient reads out, and that an
// eavesdropper cannot decrypt.
int main() {
    const Identity alice = generate_identity();
    const Identity bob = generate_identity();
    const Identity eve = generate_identity();

    std::cout << "Identities (fingerprints):\n"
              << "  alice = " << short_fp(alice) << "\n"
              << "  bob   = " << short_fp(bob) << "\n"
              << "  eve   = " << short_fp(eve) << " (eavesdropper)\n\n";

    // --- What crosses the wire ------------------------------------------------
    Session a_to_b(alice.public_key, alice.secret_key, bob.public_key);
    Session b_from_a(bob.public_key, bob.secret_key, alice.public_key);

    const std::string plaintext = "Hello Bob — this is end-to-end encrypted.";
    const auto packet = a_to_b.encrypt(to_bytes(plaintext));
    const auto wire = openmesh::protocol::serialize(packet);

    std::cout << "Alice sends: \"" << plaintext << "\"\n\n";
    std::cout << "On the wire (" << wire.size() << " bytes — what a relay/observer sees):\n  "
              << openmesh::core::to_hex(wire) << "\n";
    std::cout << "  type=MESSAGE  counter=" << packet.counter
              << "  payload is ciphertext, not text.\n\n";

    // --- Recipient decrypts ---------------------------------------------------
    auto opened = b_from_a.decrypt(packet);
    std::cout << "Bob decrypts: \"" << (opened ? to_string(*opened) : "<FAILED>") << "\"\n";

    // --- Eavesdropper cannot ---------------------------------------------------
    Session eve_from_alice(eve.public_key, eve.secret_key, alice.public_key);
    auto eve_attempt = eve_from_alice.decrypt(packet);
    std::cout << "Eve decrypts: " << (eve_attempt ? "READ IT (BUG!)" : "cannot read it ✓")
              << "\n\n";

    // --- The same, but actually over UDP via the Engine -----------------------
    Engine alice_engine(alice.public_key, alice.secret_key);
    Engine bob_engine(bob.public_key, bob.secret_key);
    alice_engine.bind(Endpoint{"127.0.0.1", 0});
    bob_engine.bind(Endpoint{"127.0.0.1", 0});
    alice_engine.add_peer(bob.public_key, *bob_engine.local_endpoint());
    bob_engine.add_peer(alice.public_key, *alice_engine.local_endpoint());

    std::string delivered;
    bob_engine.on_message([&](const Message& m) { delivered = to_string(m.plaintext); });
    alice_engine.send(bob.public_key, to_bytes("Delivered over UDP loopback."));
    bob_engine.poll(/*timeout_ms=*/1000);

    std::cout << "Over UDP, Bob received: \"" << delivered << "\"\n";
    return (opened && !eve_attempt && !delivered.empty()) ? 0 : 1;
}
