#include "openmesh/crypto/identity.hpp"
#include "openmesh/crypto/sign.hpp"
#include "openmesh/messaging/engine.hpp"
#include "openmesh/net/signaling_client.hpp"
#include "server.hpp"

#include <iostream>
#include <string>
#include <thread>

using openmesh::crypto::generate_identity;
using openmesh::crypto::Identity;
using openmesh::messaging::Bytes;
using openmesh::messaging::Engine;
using openmesh::messaging::Message;
using openmesh::net::Endpoint;
using openmesh::net::SignalingClient;
using openmesh::signaling::SignalingServer;

namespace {
Bytes to_bytes(const std::string& s) {
    return Bytes(s.begin(), s.end());
}
std::string to_string(const Bytes& b) {
    return std::string(b.begin(), b.end());
}
std::string fp(const Identity& id) {
    return id.fingerprint().substr(0, 16) + "…";
}

SignalingClient::Signer signer_for(const Identity& id) {
    return [id](const Bytes& challenge) {
        return openmesh::crypto::sign_detached(challenge, id.secret_key);
    };
}
} // namespace

// Two peers who only know each other's public keys connect through a public
// signaling server and exchange an end-to-end encrypted message — the whole
// stack in one process, over one shared socket per peer.
int main() {
    SignalingServer server;
    server.bind(Endpoint{"127.0.0.1", 0});
    const auto server_ep = server.local_endpoint();
    std::thread server_thread([&] { server.run(); });
    std::cout << "Signaling server listening on " << server_ep->to_string() << "\n\n";

    const Identity alice = generate_identity();
    const Identity bob = generate_identity();
    std::cout << "alice = " << fp(alice) << "\nbob   = " << fp(bob) << "\n\n";

    Engine alice_engine(alice.public_key, alice.secret_key);
    Engine bob_engine(bob.public_key, bob.secret_key);
    alice_engine.bind(Endpoint{"127.0.0.1", 0});
    bob_engine.bind(Endpoint{"127.0.0.1", 0});

    // Signaling shares each engine's messaging socket.
    SignalingClient alice_sig(alice_engine.transport(), *server_ep, alice.public_key,
                              signer_for(alice));
    SignalingClient bob_sig(bob_engine.transport(), *server_ep, bob.public_key, signer_for(bob));

    std::cout << "alice registers: " << (alice_sig.register_self() ? "ok" : "FAILED") << "\n";
    std::cout << "bob registers:   " << (bob_sig.register_self() ? "ok" : "FAILED") << "\n\n";

    auto bob_addr = alice_sig.discover(bob.public_key);
    std::cout << "alice discovers bob -> "
              << (bob_addr.endpoint ? bob_addr.endpoint->to_string() : "offline") << "\n";
    auto alice_addr = bob_sig.discover(alice.public_key);
    std::cout << "bob discovers alice -> "
              << (alice_addr.endpoint ? alice_addr.endpoint->to_string() : "offline") << "\n\n";

    alice_engine.add_peer(bob.public_key, *bob_addr.endpoint);
    bob_engine.add_peer(alice.public_key, *alice_addr.endpoint);

    std::string bob_got;
    bob_engine.on_message([&](const Message& m) { bob_got = to_string(m.plaintext); });
    alice_engine.send(bob.public_key,
                      to_bytes("Hi Bob — encrypted, via a server that can't read this."));
    bob_engine.poll(1000);
    std::cout << "alice -> bob (decrypted by bob): \"" << bob_got << "\"\n";

    std::string alice_got;
    alice_engine.on_message([&](const Message& m) { alice_got = to_string(m.plaintext); });
    bob_engine.send(alice.public_key, to_bytes("Got it, Alice."));
    alice_engine.poll(1000);
    std::cout << "bob -> alice (decrypted by alice): \"" << alice_got << "\"\n";

    server.stop();
    server_thread.join();

    const bool ok = !bob_got.empty() && !alice_got.empty();
    std::cout << "\n"
              << (ok ? "Two strangers connected and chatted, end-to-end encrypted." : "FAILED")
              << "\n";
    return ok ? 0 : 1;
}
