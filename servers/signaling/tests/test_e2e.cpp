#include "openmesh/crypto/identity.hpp"
#include "openmesh/crypto/sign.hpp"
#include "openmesh/net/signaling_client.hpp"
#include "server.hpp"

#include <cassert>
#include <thread>

using namespace openmesh;
using net::SignalingClient;
using signaling::SignalingServer;

namespace {
// Build a Signer closure that signs challenges with an identity's secret key.
SignalingClient::Signer signer_for(const crypto::Identity& id) {
    return [id](const net::Bytes& challenge) {
        return crypto::sign_detached(challenge, id.secret_key);
    };
}
} // namespace

// Success Criterion #1: two peers register with a signaling server and discover
// each other, exercising the real SignalingClient against the real
// SignalingServer over UDP loopback.
int main() {
    SignalingServer server;
    assert(server.bind(net::Endpoint{"127.0.0.1", 0}));
    const auto server_ep = server.local_endpoint();
    assert(server_ep.has_value());

    // Run the server loop on a background thread.
    std::thread server_thread([&] { server.run(); });

    const crypto::Identity alice = crypto::generate_identity();
    const crypto::Identity bob = crypto::generate_identity();

    SignalingClient alice_client(*server_ep, alice.public_key, signer_for(alice));
    SignalingClient bob_client(*server_ep, bob.public_key, signer_for(bob));

    // Both register (proof-of-ownership handshake over real sockets).
    assert(alice_client.register_self());
    assert(bob_client.register_self());

    // Alice discovers Bob -> gets Bob's actual registered port.
    auto found_bob = alice_client.discover(bob.public_key);
    assert(found_bob.responded);
    assert(found_bob.endpoint.has_value());
    assert(found_bob.endpoint->port == bob_client.local_endpoint()->port);

    // Bob discovers Alice -> gets Alice's actual registered port.
    auto found_alice = bob_client.discover(alice.public_key);
    assert(found_alice.responded);
    assert(found_alice.endpoint.has_value());
    assert(found_alice.endpoint->port == alice_client.local_endpoint()->port);

    // Discovering an unregistered identity: server responds, peer offline.
    auto missing = alice_client.discover(crypto::generate_identity().public_key);
    assert(missing.responded);
    assert(!missing.endpoint.has_value());

    // Heartbeat keepalive is answered.
    assert(alice_client.heartbeat());

    // Alice leaves; Bob can no longer find her.
    alice_client.disconnect();
    auto gone = bob_client.discover(alice.public_key);
    assert(gone.responded);
    assert(!gone.endpoint.has_value());

    server.stop();
    server_thread.join();
    return 0;
}
