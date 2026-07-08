#include "openmesh/protocol/packet.hpp"

#include <cassert>

int main() {
    using namespace openmesh::protocol;

    Packet in;
    in.type = PacketType::Message;
    in.payload = {0xDE, 0xAD, 0xBE, 0xEF};

    const auto bytes = serialize(in);

    Packet out;
    assert(deserialize(bytes, out));
    assert(out.version == in.version);
    assert(out.type == PacketType::Message);
    assert(out.payload == in.payload);

    // Too-short input must be rejected.
    Packet ignored;
    assert(!deserialize({0x01}, ignored));

    assert(to_string(PacketType::ContactRequest) == "CONTACT_REQUEST");
    return 0;
}
