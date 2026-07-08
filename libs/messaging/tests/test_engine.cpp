#include "openmesh/crypto/identity.hpp"
#include "openmesh/messaging/engine.hpp"

#include <cassert>
#include <string>

using namespace openmesh::messaging;
using openmesh::crypto::generate_identity;
using openmesh::crypto::Identity;
using openmesh::net::Endpoint;

static Bytes bytes(const char* s) {
    return Bytes(s, s + std::char_traits<char>::length(s));
}

// Two engines exchange a real encrypted message over UDP loopback — the whole
// stack (identity -> key agreement -> AEAD -> wire format -> socket) end to end.
int main() {
    const Identity alice = generate_identity();
    const Identity bob = generate_identity();

    Engine alice_engine(alice.public_key, alice.secret_key);
    Engine bob_engine(bob.public_key, bob.secret_key);
    assert(alice_engine.bind(Endpoint{"127.0.0.1", 0}));
    assert(bob_engine.bind(Endpoint{"127.0.0.1", 0}));

    const auto alice_ep = alice_engine.local_endpoint();
    const auto bob_ep = bob_engine.local_endpoint();
    assert(alice_ep && bob_ep);

    // Each side knows the other's identity and address (contact flow is elsewhere).
    assert(alice_engine.add_peer(bob.public_key, *bob_ep));
    assert(bob_engine.add_peer(alice.public_key, *alice_ep));
    assert(alice_engine.knows_peer(bob.public_key));

    // Alice -> Bob.
    Bytes received;
    Bytes from;
    bob_engine.on_message([&](const Message& m) {
        received = m.plaintext;
        from = m.peer;
    });
    assert(alice_engine.send(bob.public_key, bytes("hello over the wire")));
    assert(bob_engine.poll(/*timeout_ms=*/1000));
    assert(received == bytes("hello over the wire"));
    assert(from == alice.public_key);

    // Bob -> Alice reply.
    Bytes reply;
    alice_engine.on_message([&](const Message& m) { reply = m.plaintext; });
    assert(bob_engine.send(alice.public_key, bytes("got it")));
    assert(alice_engine.poll(1000));
    assert(reply == bytes("got it"));

    // Sending to an unknown peer fails cleanly.
    assert(!alice_engine.send(generate_identity().public_key, bytes("nobody")));
    return 0;
}
