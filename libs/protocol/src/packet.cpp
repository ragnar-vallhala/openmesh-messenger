#include "openmesh/protocol/packet.hpp"

namespace openmesh::protocol {

// Placeholder framing: [version][type][payload...].
// Replace with the specified wire layout (docs/protocol/) including length
// prefixes and authentication material.
std::vector<std::uint8_t> serialize(const Packet& packet) {
    std::vector<std::uint8_t> out;
    out.reserve(2 + packet.payload.size());
    out.push_back(packet.version);
    out.push_back(static_cast<std::uint8_t>(packet.type));
    out.insert(out.end(), packet.payload.begin(), packet.payload.end());
    return out;
}

bool deserialize(const std::vector<std::uint8_t>& bytes, Packet& out) {
    if (bytes.size() < 2) {
        return false;
    }
    out.version = bytes[0];
    out.type = static_cast<PacketType>(bytes[1]);
    out.payload.assign(bytes.begin() + 2, bytes.end());
    return true;
}

} // namespace openmesh::protocol
