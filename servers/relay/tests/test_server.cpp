#include "openmesh/protocol/packet.hpp"
#include "server.hpp"

#include <cassert>

using namespace openmesh::relay;
using openmesh::net::Bytes;
using openmesh::net::Endpoint;
using openmesh::protocol::Packet;
using openmesh::protocol::PacketType;

namespace {
Bytes announce(const Bytes& who) {
    Packet p;
    p.type = PacketType::Hello;
    p.source = who; // no destination
    return openmesh::protocol::serialize(p);
}
Bytes message(const Bytes& from, const Bytes& to) {
    Packet p;
    p.type = PacketType::Message;
    p.flags = openmesh::protocol::kFlagEncrypted;
    p.source = from;
    p.destination = to;
    p.payload = {0xDE, 0xAD};
    return openmesh::protocol::serialize(p);
}
} // namespace

int main() {
    RelayServer relay;
    const Bytes alice = {0xA1};
    const Bytes bob = {0xB2};
    const Endpoint alice_ep{"192.0.2.1", 5000};
    const Endpoint bob_ep{"192.0.2.2", 6000};

    std::vector<Forward> out;

    // A message to an unannounced destination is dropped (no route to Bob yet),
    // though the relay does learn Alice's route from the packet's source.
    relay.handle(message(alice, bob), alice_ep, 100, out);
    assert(out.empty());
    assert(relay.routes().lookup("a1").has_value());

    // Announce learns Bob's route too, forwarding nothing.
    out.clear();
    relay.handle(announce(bob), bob_ep, 100, out);
    assert(out.empty());
    assert(relay.routes().size() == 2);

    // Now a message from Alice to Bob is forwarded verbatim to Bob's endpoint.
    out.clear();
    const Bytes msg = message(alice, bob);
    relay.handle(msg, alice_ep, 101, out);
    assert(out.size() == 1);
    assert(out[0].to.host == "192.0.2.2" && out[0].to.port == 6000);
    assert(out[0].datagram == msg); // forwarded unchanged (content-agnostic)

    // Garbage that isn't a valid packet is dropped.
    out.clear();
    relay.handle(Bytes{0x00, 0x01, 0x02}, alice_ep, 102, out);
    assert(out.empty());

    // Routes expire once now - ttl passes their last-seen (Alice was refreshed
    // to t=101 by the forwarded message above).
    relay.expire(/*now=*/200); // cutoff = 200 - 60 = 140 > 101
    assert(relay.routes().size() == 0);
    return 0;
}
