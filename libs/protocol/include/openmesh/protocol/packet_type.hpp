#pragma once

#include <cstdint>
#include <string_view>

namespace openmesh::protocol {

// Wire packet types (SRS §9). Values are fixed on the wire — do not renumber
// existing entries; append new types at the end.
enum class PacketType : std::uint8_t {
    Hello           = 0x01,
    Register        = 0x02,
    Discover        = 0x03,
    Connect         = 0x04,
    ContactRequest  = 0x05,
    ContactResponse = 0x06,
    Message         = 0x07,
    Ack             = 0x08,
    Heartbeat       = 0x09,
    Disconnect      = 0x0A,
};

constexpr std::string_view to_string(PacketType type) {
    switch (type) {
    case PacketType::Hello:           return "HELLO";
    case PacketType::Register:        return "REGISTER";
    case PacketType::Discover:        return "DISCOVER";
    case PacketType::Connect:         return "CONNECT";
    case PacketType::ContactRequest:  return "CONTACT_REQUEST";
    case PacketType::ContactResponse: return "CONTACT_RESPONSE";
    case PacketType::Message:         return "MESSAGE";
    case PacketType::Ack:             return "ACK";
    case PacketType::Heartbeat:       return "HEARTBEAT";
    case PacketType::Disconnect:      return "DISCONNECT";
    }
    return "UNKNOWN";
}

} // namespace openmesh::protocol
