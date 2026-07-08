#include "openmesh/crypto/identity.hpp"
#include "openmesh/messaging/engine.hpp"
#include "openmesh/messaging/session.hpp"
#include "openmesh/net/udp_socket.hpp"
#include "openmesh/protocol/packet.hpp"

#include <cassert>
#include <string>

using namespace openmesh::messaging;
using openmesh::crypto::generate_identity;
using openmesh::crypto::Identity;
using openmesh::net::Endpoint;
using openmesh::storage::TrustStatus;

namespace {
Bytes bytes(const char* s) {
    return Bytes(s, s + std::char_traits<char>::length(s));
}

struct Peer {
    Identity id;
    Engine engine;
    explicit Peer(Identity i) : id(std::move(i)), engine(id.public_key, id.secret_key) {
        assert(engine.bind(Endpoint{"127.0.0.1", 0}));
    }
    Endpoint endpoint() const { return *engine.local_endpoint(); }
};
} // namespace

// Full contact-request handshake: Alice requests, Bob accepts, then they can
// message; before acceptance, messaging is blocked (SRS FR-2, §7).
static void test_request_accept_then_message() {
    Peer alice(generate_identity());
    Peer bob(generate_identity());

    Bytes req_from;
    Bytes req_greeting;
    bob.engine.on_contact_request([&](const Bytes& peer, const Bytes& greeting) {
        req_from = peer;
        req_greeting = greeting;
    });

    // Alice -> contact request.
    assert(
        alice.engine.send_contact_request(bob.id.public_key, bob.endpoint(), bytes("hi, Alice")));
    assert(alice.engine.trust_of(bob.id.public_key) == TrustStatus::Pending);

    // Before acceptance, Alice cannot message Bob.
    assert(!alice.engine.send(bob.id.public_key, bytes("too early")));

    // Bob receives the request.
    assert(bob.engine.poll(1000));
    assert(req_from == alice.id.public_key);
    assert(req_greeting == bytes("hi, Alice"));

    // Bob accepts -> both become accepted.
    bool accepted_notice = false;
    alice.engine.on_contact_response([&](const Bytes& peer, bool accepted) {
        accepted_notice = accepted && peer == bob.id.public_key;
    });
    assert(bob.engine.accept_contact(alice.id.public_key));
    assert(bob.engine.trust_of(alice.id.public_key) == TrustStatus::Accepted);
    assert(alice.engine.poll(1000)); // receives CONTACT_RESPONSE(accept)
    assert(accepted_notice);
    assert(alice.engine.trust_of(bob.id.public_key) == TrustStatus::Accepted);

    // Now messaging works both ways.
    Bytes got;
    bob.engine.on_message([&](const Message& m) { got = m.plaintext; });
    assert(alice.engine.send(bob.id.public_key, bytes("hello contact")));
    assert(bob.engine.poll(1000));
    assert(got == bytes("hello contact"));
}

// Rejection: after Bob rejects, neither side can message.
static void test_request_reject() {
    Peer alice(generate_identity());
    Peer bob(generate_identity());

    bob.engine.on_contact_request([&](const Bytes&, const Bytes&) {});
    assert(alice.engine.send_contact_request(bob.id.public_key, bob.endpoint(), {}));
    assert(bob.engine.poll(1000));

    bool rejected_notice = false;
    alice.engine.on_contact_response(
        [&](const Bytes&, bool accepted) { rejected_notice = !accepted; });
    assert(bob.engine.reject_contact(alice.id.public_key));
    assert(bob.engine.trust_of(alice.id.public_key) == TrustStatus::Rejected);
    assert(alice.engine.poll(1000));
    assert(rejected_notice);
    assert(alice.engine.trust_of(bob.id.public_key) == TrustStatus::Rejected);

    // No messaging after rejection.
    assert(!alice.engine.send(bob.id.public_key, bytes("please")));
}

// The one-request rule: a second contact request from the same peer is not
// surfaced again (SRS §7).
static void test_one_request_rule() {
    Peer alice(generate_identity());
    Peer bob(generate_identity());

    int surfaced = 0;
    bob.engine.on_contact_request([&](const Bytes&, const Bytes&) { ++surfaced; });

    assert(alice.engine.send_contact_request(bob.id.public_key, bob.endpoint(), bytes("first")));
    assert(bob.engine.poll(1000));
    // Alice sends another request (e.g. a retry); Bob must not surface it twice.
    assert(alice.engine.send_contact_request(bob.id.public_key, bob.endpoint(), bytes("second")));
    bob.engine.poll(500);
    assert(surfaced == 1);
}

// Spam gate at the receiver: a MESSAGE from a non-accepted peer is dropped, even
// if the attacker crafts a valid encrypted packet (bypassing Engine::send).
static void test_message_from_stranger_dropped() {
    Peer bob(generate_identity());
    const Identity mallory = generate_identity();

    // Mallory crafts a valid encrypted MESSAGE to Bob directly via a raw session.
    Session m_to_bob(mallory.public_key, mallory.secret_key, bob.id.public_key);
    const auto packet = m_to_bob.encrypt(bytes("unsolicited"));

    openmesh::net::UdpSocket raw;
    assert(raw.send_to(bob.endpoint(), openmesh::protocol::serialize(packet)));

    bool delivered = false;
    bob.engine.on_message([&](const Message&) { delivered = true; });
    bob.engine.poll(500); // should drop it (Mallory is not an accepted contact)
    assert(!delivered);
    assert(!bob.engine.trust_of(mallory.public_key).has_value());
}

int main() {
    test_request_accept_then_message();
    test_request_reject();
    test_one_request_rule();
    test_message_from_stranger_dropped();
    return 0;
}
