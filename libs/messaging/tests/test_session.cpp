#include "openmesh/crypto/identity.hpp"
#include "openmesh/messaging/session.hpp"

#include <cassert>
#include <string>

using namespace openmesh::messaging;
namespace protocol = openmesh::protocol;
using openmesh::crypto::generate_identity;
using openmesh::crypto::Identity;

static Session session_between(const Identity& me, const Identity& peer) {
    return Session(me.public_key, me.secret_key, peer.public_key);
}

static Bytes bytes(const char* s) {
    return Bytes(s, s + std::char_traits<char>::length(s));
}

static void test_roundtrip_both_ways() {
    const Identity alice = generate_identity();
    const Identity bob = generate_identity();
    Session a = session_between(alice, bob);
    Session b = session_between(bob, alice);
    assert(a.valid() && b.valid());

    // Alice -> Bob.
    protocol::Packet p = a.encrypt(bytes("hello bob"));
    assert(p.type == protocol::PacketType::Message);
    assert(p.encrypted());
    assert(p.source == alice.public_key && p.destination == bob.public_key);
    assert(p.counter == 0);
    assert(p.payload != bytes("hello bob")); // actually encrypted
    assert(p.payload.size() > 9);            // nonce + tag overhead

    auto opened = b.decrypt(p);
    assert(opened.has_value() && *opened == bytes("hello bob"));

    // Bob -> Alice (independent counter/direction).
    protocol::Packet q = b.encrypt(bytes("hi alice"));
    auto opened2 = a.decrypt(q);
    assert(opened2.has_value() && *opened2 == bytes("hi alice"));
}

static void test_counter_replay_and_ordering() {
    const Identity alice = generate_identity();
    const Identity bob = generate_identity();
    Session a = session_between(alice, bob);
    Session b = session_between(bob, alice);

    protocol::Packet m0 = a.encrypt(bytes("m0"));
    protocol::Packet m1 = a.encrypt(bytes("m1"));
    assert(m0.counter == 0 && m1.counter == 1);

    assert(b.decrypt(m1).has_value());  // accept counter 1
    assert(!b.decrypt(m0).has_value()); // old counter rejected (ordering)
    assert(!b.decrypt(m1).has_value()); // exact replay rejected
}

static void test_tamper_rejected() {
    const Identity alice = generate_identity();
    const Identity bob = generate_identity();
    Session a = session_between(alice, bob);

    // Tampered ciphertext fails authentication.
    {
        Session b = session_between(bob, alice);
        protocol::Packet p = a.encrypt(bytes("secret"));
        p.payload.back() ^= 0xFF;
        assert(!b.decrypt(p).has_value());
    }
    // Tampered envelope (counter is bound as AAD): passes the counter check but
    // fails AEAD authentication.
    {
        Session b = session_between(bob, alice);
        protocol::Packet p = a.encrypt(bytes("secret"));
        p.counter = 99; // AAD no longer matches what was sealed
        assert(!b.decrypt(p).has_value());
    }
}

static void test_wrong_recipients() {
    const Identity alice = generate_identity();
    const Identity bob = generate_identity();
    const Identity eve = generate_identity();

    protocol::Packet p = session_between(alice, bob).encrypt(bytes("for bob only"));

    // Eve holds a session with Alice but is not the addressed recipient, and her
    // shared key differs — she cannot read it.
    Session eve_with_alice = session_between(eve, alice);
    assert(!eve_with_alice.decrypt(p).has_value());

    // A message not addressed to us is rejected by the recipient check.
    Session bob_with_eve = session_between(bob, eve);
    assert(!bob_with_eve.decrypt(p).has_value());
}

int main() {
    test_roundtrip_both_ways();
    test_counter_replay_and_ordering();
    test_tamper_rejected();
    test_wrong_recipients();
    return 0;
}
