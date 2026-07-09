#include "openmesh/crypto/identity.hpp"
#include "openmesh/messaging/engine.hpp"

#include <cassert>

using namespace openmesh::messaging;
using openmesh::crypto::generate_identity;
using openmesh::crypto::Identity;
using openmesh::net::Endpoint;

// Simulate a UDP hole punch over loopback: two engines probe each other's
// endpoints and both confirm a direct path. (Real NAT can't be simulated in a
// unit test, but this exercises the probe/confirm handshake and the upgrade.)
static void test_direct_punch() {
    const Identity alice = generate_identity();
    const Identity bob = generate_identity();

    Engine ae(alice.public_key, alice.secret_key);
    Engine be(bob.public_key, bob.secret_key);
    assert(ae.bind(Endpoint{"127.0.0.1", 0}));
    assert(be.bind(Endpoint{"127.0.0.1", 0}));
    const auto a_ep = *ae.local_endpoint();
    const auto b_ep = *be.local_endpoint();

    // Both start probing each other's endpoints (as they would after exchanging
    // candidates via signaling).
    ae.connect(bob.public_key, b_ep);
    be.connect(alice.public_key, a_ep);
    assert(!ae.has_direct_path(bob.public_key));

    // Drive the exchange: each side receives the other's probe and confirms.
    for (int i = 0; i < 6; ++i) {
        ae.poll(100);
        be.poll(100);
    }
    assert(ae.has_direct_path(bob.public_key));
    assert(be.has_direct_path(alice.public_key));

    // With a direct path, accepted contacts can message directly.
    assert(ae.add_peer(bob.public_key, b_ep));
    assert(be.add_peer(alice.public_key, a_ep));
    Bytes got;
    be.on_message([&](const Message& m) { got = m.plaintext; });
    assert(ae.send(bob.public_key, Bytes{'h', 'i'}));
    assert(be.poll(500));
    assert((got == Bytes{'h', 'i'}));
}

// With a relay configured, an unreachable candidate falls back to the relay: the
// active endpoint is the relay and no direct path is confirmed.
static void test_relay_fallback() {
    const Identity alice = generate_identity();
    const Identity bob = generate_identity();

    Engine ae(alice.public_key, alice.secret_key);
    assert(ae.bind(Endpoint{"127.0.0.1", 0}));

    const Endpoint relay{"198.51.100.9", 4434};
    ae.set_relay(relay);

    // A candidate that never answers (nothing listening on this port).
    const Endpoint dead{"127.0.0.1", 9};
    const Endpoint active = ae.connect(bob.public_key, dead);
    assert(active.host == relay.host && active.port == relay.port); // routed via relay

    for (int i = 0; i < 5; ++i) {
        ae.tick();
        ae.poll(50);
    }
    assert(!ae.has_direct_path(bob.public_key)); // no direct path came up
}

int main() {
    test_direct_punch();
    test_relay_fallback();
    return 0;
}
