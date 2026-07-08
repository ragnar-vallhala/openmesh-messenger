#pragma once

#include "openmesh/protocol/packet_type.hpp"

#include <cstdint>
#include <vector>

namespace openmesh::protocol {

// Current wire protocol version. Bumped on incompatible envelope changes.
inline constexpr std::uint8_t kProtocolVersion = 1;

// A parsed packet envelope. The `payload` is opaque bytes: for MESSAGE it is
// ciphertext that servers must never be able to read (SRS §5, §6, §8).
//
// NOTE: This is an early skeleton. The concrete on-wire byte layout (header
// fields, lengths, endianness, auth tag placement) is specified in
// docs/protocol/ and will be filled in here.
struct Packet {
    std::uint8_t version = kProtocolVersion;
    PacketType   type    = PacketType::Hello;
    std::vector<std::uint8_t> payload;
};

// Serialize a packet to its wire representation.
std::vector<std::uint8_t> serialize(const Packet& packet);

// Parse a packet from wire bytes. Returns false on malformed input.
bool deserialize(const std::vector<std::uint8_t>& bytes, Packet& out);

} // namespace openmesh::protocol
