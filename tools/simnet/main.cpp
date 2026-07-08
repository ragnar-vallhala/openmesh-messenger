#include "openmesh/net/udp_socket.hpp"
#include "openmesh/protocol/packet.hpp"

#include <iostream>

// om-simnet: exercise the wire format over a real UDP loopback socket — build a
// packet, serialize it, send it to ourselves, receive and parse it back. A small
// end-to-end sanity check of the protocol + socket layers.
int main() {
    using namespace openmesh::protocol;
    using openmesh::net::Endpoint;
    using openmesh::net::UdpSocket;

    UdpSocket sock;
    if (!sock.bind(Endpoint{"127.0.0.1", 0})) {
        std::cerr << "bind failed\n";
        return 1;
    }
    const auto local = sock.local_endpoint();
    if (!local) {
        std::cerr << "local_endpoint failed\n";
        return 1;
    }

    Packet out;
    out.type = PacketType::Hello;
    out.counter = 1;
    out.payload = {'o', 'm'};

    const auto wire = serialize(out);
    if (!sock.send_to(*local, wire)) {
        std::cerr << "send failed\n";
        return 1;
    }

    const auto dg = sock.receive(/*timeout_ms=*/1000);
    if (!dg) {
        std::cerr << "receive timed out\n";
        return 1;
    }

    Packet in;
    if (!deserialize(dg->data, in)) {
        std::cerr << "parse failed\n";
        return 1;
    }

    std::cout << "UDP loopback ok on " << local->to_string() << ": " << to_string(in.type) << " ("
              << dg->data.size() << " bytes) from " << dg->from.to_string() << '\n';
    return 0;
}
