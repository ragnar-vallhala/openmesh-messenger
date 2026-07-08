#include "openmesh/crypto/identity.hpp"
#include "openmesh/crypto/sign.hpp"
#include "openmesh/messaging/engine.hpp"
#include "openmesh/net/signaling_client.hpp"
#include "server.hpp"

#include <cassert>
#include <string>
#include <thread>

using namespace openmesh;
using messaging::Engine;
using messaging::Message;
using net::SignalingClient;
using signaling::SignalingServer;

namespace {
SignalingClient::Signer signer_for(const crypto::Identity& id) {
    return [id](const net::Bytes& challenge) {
        return crypto::sign_detached(challenge, id.secret_key);
    };
}
messaging::Bytes bytes(const char* s) {
    return messaging::Bytes(s, s + std::char_traits<char>::length(s));
}
} // namespace

// The whole promise, end to end: two peers that only know each other's public
// keys register with a public signaling server, discover each other's address,
// and exchange an end-to-end encrypted message — over ONE shared socket each, so
// the endpoint the server registers is exactly where messages land.
int main() {
    SignalingServer server;
    assert(server.bind(net::Endpoint{"127.0.0.1", 0}));
    const auto server_ep = server.local_endpoint();
    assert(server_ep.has_value());
    std::thread server_thread([&] { server.run(); });

    const crypto::Identity alice = crypto::generate_identity();
    const crypto::Identity bob = crypto::generate_identity();

    // Each peer has ONE Engine (its messaging transport); signaling shares that
    // very socket via Engine::transport().
    Engine alice_engine(alice.public_key, alice.secret_key);
    Engine bob_engine(bob.public_key, bob.secret_key);
    assert(alice_engine.bind(net::Endpoint{"127.0.0.1", 0}));
    assert(bob_engine.bind(net::Endpoint{"127.0.0.1", 0}));

    SignalingClient alice_sig(alice_engine.transport(), *server_ep, alice.public_key,
                              signer_for(alice));
    SignalingClient bob_sig(bob_engine.transport(), *server_ep, bob.public_key, signer_for(bob));

    // 1. Both register their (shared) messaging endpoint with the server.
    assert(alice_sig.register_self());
    assert(bob_sig.register_self());

    // 2. Discover each other by public key -> real messaging endpoints.
    auto bob_addr = alice_sig.discover(bob.public_key);
    auto alice_addr = bob_sig.discover(alice.public_key);
    assert(bob_addr.endpoint.has_value());
    assert(alice_addr.endpoint.has_value());

    // 3. Establish sessions at the discovered addresses.
    assert(alice_engine.add_peer(bob.public_key, *bob_addr.endpoint));
    assert(bob_engine.add_peer(alice.public_key, *alice_addr.endpoint));

    // 4. Exchange an encrypted message that actually reaches the peer, because
    //    the discovered address is the peer's messaging socket.
    messaging::Bytes got;
    bob_engine.on_message([&](const Message& m) { got = m.plaintext; });
    assert(alice_engine.send(bob.public_key, bytes("hello from a stranger")));
    assert(bob_engine.poll(/*timeout_ms=*/1000));
    assert(got == bytes("hello from a stranger"));

    // 5. Reply the other way.
    messaging::Bytes reply;
    alice_engine.on_message([&](const Message& m) { reply = m.plaintext; });
    assert(bob_engine.send(alice.public_key, bytes("hello back")));
    assert(alice_engine.poll(1000));
    assert(reply == bytes("hello back"));

    server.stop();
    server_thread.join();
    return 0;
}
