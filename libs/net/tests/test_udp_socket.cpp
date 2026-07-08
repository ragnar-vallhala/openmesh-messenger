#include "openmesh/net/udp_socket.hpp"
#include "openmesh/protocol/packet.hpp"

#include <cassert>

using namespace openmesh::net;

static void test_loopback_roundtrip() {
    // Receiver binds to an ephemeral loopback port.
    UdpSocket receiver;
    assert(receiver.bind(Endpoint{"127.0.0.1", 0}));
    auto local = receiver.local_endpoint();
    assert(local.has_value());
    assert(local->host == "127.0.0.1");
    assert(local->port != 0);

    // Sender (unbound) sends a datagram; it auto-binds to an ephemeral port.
    UdpSocket sender;
    const Bytes ping = {'p', 'i', 'n', 'g'};
    assert(sender.send_to(*local, ping));

    auto dg = receiver.receive(/*timeout_ms=*/1000);
    assert(dg.has_value());
    assert(dg->data == ping);
    assert(dg->from.host == "127.0.0.1");
    assert(dg->from.port != 0);

    // Reply to the sender's observed source address.
    const Bytes pong = {'p', 'o', 'n', 'g'};
    assert(receiver.send_to(dg->from, pong));
    auto back = sender.receive(/*timeout_ms=*/1000);
    assert(back.has_value());
    assert(back->data == pong);
}

static void test_timeout() {
    UdpSocket sock;
    assert(sock.bind(Endpoint{"127.0.0.1", 0}));
    // Nothing sent -> receive must time out and return nullopt.
    assert(!sock.receive(/*timeout_ms=*/50).has_value());
}

static void test_wire_integration() {
    // A serialized protocol packet survives a UDP round-trip and parses back.
    UdpSocket receiver;
    assert(receiver.bind(Endpoint{"127.0.0.1", 0}));
    auto local = receiver.local_endpoint();
    assert(local.has_value());

    using namespace openmesh::protocol;
    Packet out;
    out.type = PacketType::Heartbeat;
    out.counter = 42;
    out.source = {0xAB, 0xCD};

    UdpSocket sender;
    assert(sender.send_to(*local, serialize(out)));

    auto dg = receiver.receive(/*timeout_ms=*/1000);
    assert(dg.has_value());

    Packet parsed;
    assert(deserialize(dg->data, parsed));
    assert(parsed.type == PacketType::Heartbeat);
    assert(parsed.counter == 42);
    assert(parsed.source == out.source);
}

int main() {
    test_loopback_roundtrip();
    test_timeout();
    test_wire_integration();
    return 0;
}
