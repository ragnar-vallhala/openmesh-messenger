#include "openmesh/crypto/identity.hpp"
#include "openmesh/messaging/engine.hpp"
#include "openmesh/net/endpoint.hpp"

#include <iostream>
#include <string>

using openmesh::crypto::generate_identity;
using openmesh::crypto::Identity;
using openmesh::messaging::Bytes;
using openmesh::messaging::Engine;
using openmesh::messaging::Message;
using openmesh::net::Endpoint;

namespace {
Bytes to_bytes(const std::string& s) {
    return Bytes(s.begin(), s.end());
}
std::string to_string(const Bytes& b) {
    return std::string(b.begin(), b.end());
}
} // namespace

// Demonstrates the contact-request flow (SRS FR-2): an unknown user must ask
// before they can message, the recipient accepts or rejects, and only accepted
// contacts can exchange messages.
int main() {
    const Identity alice = generate_identity();
    const Identity bob = generate_identity();
    const Identity mallory = generate_identity();

    Engine alice_engine(alice.public_key, alice.secret_key);
    Engine bob_engine(bob.public_key, bob.secret_key);
    Engine mallory_engine(mallory.public_key, mallory.secret_key);
    alice_engine.bind(Endpoint{"127.0.0.1", 0});
    bob_engine.bind(Endpoint{"127.0.0.1", 0});
    mallory_engine.bind(Endpoint{"127.0.0.1", 0});
    const auto bob_ep = *bob_engine.local_endpoint();

    bob_engine.on_contact_request([&](const Bytes& peer, const Bytes& greeting) {
        std::cout << "Bob receives a contact request: \"" << to_string(greeting) << "\"\n";
        (void) peer;
    });

    // --- Alice: request -> accepted ------------------------------------------
    std::cout << "Alice sends Bob a contact request.\n";
    alice_engine.send_contact_request(bob.public_key, bob_ep, to_bytes("Hi Bob, it's Alice"));
    std::cout << "Can Alice message Bob before acceptance? "
              << (alice_engine.send(bob.public_key, to_bytes("hi")) ? "yes" : "no (blocked)")
              << "\n";

    bob_engine.poll(1000); // surfaces the request
    std::cout << "Bob accepts.\n";
    bob_engine.accept_contact(alice.public_key);

    alice_engine.on_contact_response([&](const Bytes&, bool accepted) {
        std::cout << "Alice is notified: " << (accepted ? "accepted" : "rejected") << "\n";
    });
    alice_engine.poll(1000);

    std::string bob_got;
    bob_engine.on_message([&](const Message& m) { bob_got = to_string(m.plaintext); });
    alice_engine.send(bob.public_key, to_bytes("Great — now we're contacts!"));
    bob_engine.poll(1000);
    std::cout << "Alice -> Bob: \"" << bob_got << "\"\n\n";

    // --- Mallory: request -> rejected ----------------------------------------
    bob_engine.on_contact_request([&](const Bytes& peer, const Bytes&) {
        std::cout << "Bob receives a request from a stranger and rejects it.\n";
        bob_engine.reject_contact(peer);
    });
    mallory_engine.send_contact_request(bob.public_key, bob_ep, to_bytes("let me in"));
    bob_engine.poll(1000);
    mallory_engine.on_contact_response([&](const Bytes&, bool accepted) {
        std::cout << "Mallory is notified: " << (accepted ? "accepted" : "rejected") << "\n";
    });
    mallory_engine.poll(1000);
    std::cout << "Can Mallory message Bob? "
              << (mallory_engine.send(bob.public_key, to_bytes("hello?")) ? "yes" : "no (blocked)")
              << "\n";

    return bob_got.empty() ? 1 : 0;
}
