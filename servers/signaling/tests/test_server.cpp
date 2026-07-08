#include "openmesh/crypto/identity.hpp"
#include "openmesh/crypto/sign.hpp"
#include "openmesh/net/udp_socket.hpp"
#include "openmesh/protocol/packet.hpp"
#include "server.hpp"

#include <cassert>
#include <string>

using namespace openmesh;
using net::Endpoint;
using protocol::Packet;
using protocol::PacketType;
using signaling::Outgoing;
using signaling::SignalingServer;

namespace {
std::string to_hex(const net::Bytes& bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    for (std::uint8_t b : bytes) {
        out.push_back(kHex[b >> 4]);
        out.push_back(kHex[b & 0x0F]);
    }
    return out;
}

Packet reg_request(const crypto::Identity& id) {
    Packet p;
    p.type = PacketType::Register;
    p.source = id.public_key;
    return p;
}
} // namespace

// The REGISTER proof-of-ownership handshake registers a peer at its observed
// source endpoint (SRS §11), driven purely through handle() (no sockets).
static void test_register_handshake() {
    SignalingServer server;
    const crypto::Identity id = crypto::generate_identity();
    const Endpoint from{"192.0.2.1", 5000};

    // Step 1 -> challenge.
    std::vector<Outgoing> out;
    server.handle(reg_request(id), from, /*now=*/100, out);
    assert(out.size() == 1);
    assert(out[0].to.port == 5000);
    assert(out[0].packet.type == PacketType::Hello);
    const net::Bytes challenge = out[0].packet.payload;
    assert(challenge.size() == 32);

    // Step 2 (wrong signature) -> rejected, not registered.
    Packet bad = reg_request(id);
    bad.payload = crypto::sign_detached(net::Bytes{0xDE, 0xAD}, id.secret_key); // wrong message
    out.clear();
    server.handle(bad, from, 101, out);
    assert(out.empty());
    assert(!server.registry().lookup(to_hex(id.public_key)).has_value());

    // Re-issue a challenge (the bad attempt cleared the pending one).
    out.clear();
    server.handle(reg_request(id), from, 102, out);
    const net::Bytes challenge2 = out[0].packet.payload;

    // Step 2 (from a different endpoint) -> rejected (return-routability).
    Packet good = reg_request(id);
    good.payload = crypto::sign_detached(challenge2, id.secret_key);
    out.clear();
    server.handle(good, Endpoint{"192.0.2.9", 9999}, 103, out);
    assert(out.empty());
    assert(!server.registry().lookup(to_hex(id.public_key)).has_value());

    // Step 2 (correct) -> registered + ACK.
    out.clear();
    server.handle(good, from, 104, out);
    assert(out.size() == 1);
    assert(out[0].packet.type == PacketType::Ack);
    auto ep = server.registry().lookup(to_hex(id.public_key));
    assert(ep.has_value());
    assert(ep->host == "192.0.2.1" && ep->port == 5000);
}

// A registered peer, then register another and exercise DISCOVER / CONNECT.
static void test_discover_and_connect() {
    SignalingServer server;

    // Register Alice at a known endpoint via the handshake.
    const crypto::Identity alice = crypto::generate_identity();
    const Endpoint alice_ep{"198.51.100.7", 6001};
    std::vector<Outgoing> out;
    server.handle(reg_request(alice), alice_ep, 200, out);
    Packet a2 = reg_request(alice);
    a2.payload = crypto::sign_detached(out[0].packet.payload, alice.secret_key);
    out.clear();
    server.handle(a2, alice_ep, 201, out);
    assert(server.registry().lookup(to_hex(alice.public_key)).has_value());

    // DISCOVER alice -> CONNECT carrying her endpoint string.
    const Endpoint seeker{"203.0.113.5", 7000};
    Packet disc;
    disc.type = PacketType::Discover;
    disc.destination = alice.public_key;
    out.clear();
    server.handle(disc, seeker, 202, out);
    assert(out.size() == 1);
    assert(out[0].to.port == 7000);
    assert(out[0].packet.type == PacketType::Connect);
    assert(out[0].packet.source == alice.public_key);
    const std::string expected_ep = "198.51.100.7:6001";
    assert((out[0].packet.payload == net::Bytes(expected_ep.begin(), expected_ep.end())));

    // DISCOVER an unknown peer -> CONNECT with empty payload (offline).
    Packet disc_unknown;
    disc_unknown.type = PacketType::Discover;
    disc_unknown.destination = crypto::generate_identity().public_key;
    out.clear();
    server.handle(disc_unknown, seeker, 203, out);
    assert(out.size() == 1 && out[0].packet.payload.empty());

    // CONNECT from Bob addressed to Alice is relayed to Alice's endpoint verbatim.
    const crypto::Identity bob = crypto::generate_identity();
    Packet conn;
    conn.type = PacketType::Connect;
    conn.source = bob.public_key;
    conn.destination = alice.public_key;
    conn.payload = {'i', 'c', 'e'};
    out.clear();
    server.handle(conn, Endpoint{"203.0.113.9", 8000}, 204, out);
    assert(out.size() == 1);
    assert(out[0].to.host == "198.51.100.7" && out[0].to.port == 6001);
    assert(out[0].packet.source == bob.public_key);
    assert((out[0].packet.payload == net::Bytes{'i', 'c', 'e'}));
}

static void test_disconnect_and_expiry() {
    SignalingServer::Config cfg;
    cfg.peer_ttl_seconds = 60;
    cfg.challenge_ttl_seconds = 30;
    SignalingServer server{cfg};
    const crypto::Identity id = crypto::generate_identity();
    const Endpoint ep{"192.0.2.50", 5555};

    std::vector<Outgoing> out;
    server.handle(reg_request(id), ep, 300, out);
    Packet done = reg_request(id);
    done.payload = crypto::sign_detached(out[0].packet.payload, id.secret_key);
    out.clear();
    server.handle(done, ep, 301, out);
    assert(server.registry().size() == 1);

    // DISCONNECT removes the peer.
    Packet dc;
    dc.type = PacketType::Disconnect;
    dc.source = id.public_key;
    out.clear();
    server.handle(dc, ep, 302, out);
    assert(server.registry().size() == 0);

    // Expiry: register again, then run expiry well past the TTL.
    server.handle(reg_request(id), ep, 400, out);
    Packet again = reg_request(id);
    again.payload = crypto::sign_detached(out.back().packet.payload, id.secret_key);
    out.clear();
    server.handle(again, ep, 401, out);
    assert(server.registry().size() == 1);
    server.expire(/*now=*/401 + 60 + 1);
    assert(server.registry().size() == 0);
}

// End-to-end over real UDP loopback sockets, single-threaded and step-driven.
static void test_socket_handshake() {
    SignalingServer server;
    assert(server.bind(Endpoint{"127.0.0.1", 0}));
    const auto local = server.local_endpoint();
    assert(local.has_value());

    const crypto::Identity id = crypto::generate_identity();
    net::UdpSocket client;

    // Client -> REGISTER (request challenge).
    assert(client.send_to(*local, protocol::serialize(reg_request(id))));
    assert(server.run_once(/*timeout_ms=*/500, /*now=*/1000));

    // Client <- HELLO challenge.
    auto challenge_dg = client.receive(500);
    assert(challenge_dg.has_value());
    Packet challenge_pkt;
    assert(protocol::deserialize(challenge_dg->data, challenge_pkt));
    assert(challenge_pkt.type == PacketType::Hello);

    // Client -> REGISTER (signed challenge).
    Packet signed_reg = reg_request(id);
    signed_reg.payload = crypto::sign_detached(challenge_pkt.payload, id.secret_key);
    assert(client.send_to(*local, protocol::serialize(signed_reg)));
    assert(server.run_once(500, 1001));

    // Client <- ACK, and the server has registered it at the client's real port.
    auto ack_dg = client.receive(500);
    assert(ack_dg.has_value());
    Packet ack;
    assert(protocol::deserialize(ack_dg->data, ack));
    assert(ack.type == PacketType::Ack);

    const auto client_local = client.local_endpoint();
    assert(client_local.has_value());
    auto registered = server.registry().lookup(to_hex(id.public_key));
    assert(registered.has_value());
    assert(registered->port == client_local->port);
}

int main() {
    test_register_handshake();
    test_discover_and_connect();
    test_disconnect_and_expiry();
    test_socket_handshake();
    return 0;
}
