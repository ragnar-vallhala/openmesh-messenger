#include "openmesh/crypto/identity.hpp"
#include "openmesh/messaging/engine.hpp"
#include "server.hpp"

#include <cassert>
#include <string>

using namespace openmesh;
using messaging::Bytes;
using messaging::Engine;
using messaging::Message;
using relay::RelayServer;

namespace {
Bytes bytes(const char* s) {
    return Bytes(s, s + std::char_traits<char>::length(s));
}
} // namespace

// Two peers exchange end-to-end encrypted messages entirely through the relay —
// neither ever learns the other's real address; both only send to the relay,
// which forwards by destination public key. Single-threaded and step-driven.
int main() {
    RelayServer relay;
    assert(relay.bind(net::Endpoint{"127.0.0.1", 0}));
    const auto relay_ep = relay.local_endpoint();
    assert(relay_ep.has_value());

    const crypto::Identity alice = crypto::generate_identity();
    const crypto::Identity bob = crypto::generate_identity();

    Engine alice_engine(alice.public_key, alice.secret_key);
    Engine bob_engine(bob.public_key, bob.secret_key);
    assert(alice_engine.bind(net::Endpoint{"127.0.0.1", 0}));
    assert(bob_engine.bind(net::Endpoint{"127.0.0.1", 0}));

    // Both announce so the relay knows how to reach each of them.
    assert(alice_engine.announce_to(*relay_ep));
    assert(relay.run_once(200, 1000));
    assert(bob_engine.announce_to(*relay_ep));
    assert(relay.run_once(200, 1000));
    assert(relay.routes().size() == 2);

    // Trust each other (out of band) and route to the peer *via the relay*:
    // the relay's address is used as the peer endpoint.
    assert(alice_engine.add_peer(bob.public_key, *relay_ep));
    assert(bob_engine.add_peer(alice.public_key, *relay_ep));

    // Alice -> relay -> Bob.
    Bytes got;
    Bytes from;
    bob_engine.on_message([&](const Message& m) {
        got = m.plaintext;
        from = m.peer;
    });
    assert(alice_engine.send(bob.public_key, bytes("through the relay")));
    assert(relay.run_once(200, 1001)); // forward Alice's message to Bob
    assert(bob_engine.poll(500));
    assert(got == bytes("through the relay"));
    assert(from == alice.public_key); // Bob sees Alice as the sender, not the relay

    // Bob -> relay -> Alice.
    Bytes reply;
    alice_engine.on_message([&](const Message& m) { reply = m.plaintext; });
    assert(bob_engine.send(alice.public_key, bytes("got it, over relay")));
    assert(relay.run_once(200, 1002));
    assert(alice_engine.poll(500));
    assert(reply == bytes("got it, over relay"));
    return 0;
}
