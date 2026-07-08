#include "openmesh/protocol/packet.hpp"

#include <iostream>

// om-simnet: tiny protocol sanity harness — build a packet, round-trip it
// through serialize/deserialize, and print the result. A stand-in for a future
// local network simulator.
int main() {
    using namespace openmesh::protocol;

    Packet p;
    p.type = PacketType::Hello;
    p.payload = {'o', 'm'};

    const auto bytes = serialize(p);
    Packet out;
    if (!deserialize(bytes, out)) {
        std::cerr << "deserialize failed\n";
        return 1;
    }
    std::cout << "round-trip ok: " << to_string(out.type)
              << " (" << bytes.size() << " bytes)\n";
    return 0;
}
