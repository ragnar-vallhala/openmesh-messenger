#pragma once

#include "openmesh/protocol/packet_type.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

// Wire envelope framing (protocol v1). See docs/protocol/wire-format.md for the
// authoritative byte-layout specification.
namespace openmesh::protocol {

using Bytes = std::vector<std::uint8_t>;

// Magic bytes "OM" that begin every packet — a cheap filter for scan/noise
// traffic on a shared UDP port.
inline constexpr std::uint16_t kMagic = 0x4F4D;

// Current wire protocol version. Bumped only on incompatible envelope changes
// (requires an ADR).
inline constexpr std::uint8_t kProtocolVersion = 1;

// Size of the fixed header: magic(2) + version(1) + type(1) + flags(1) + counter(8).
inline constexpr std::size_t kHeaderSize = 13;

// Smallest possible packet: fixed header + three empty length-prefixed fields.
inline constexpr std::size_t kMinPacketSize = kHeaderSize + 3 * sizeof(std::uint16_t);

// Recommended maximum UDP datagram size to avoid IP fragmentation
// (IPv6 min MTU 1280 - IPv6/UDP headers). Larger messages must be chunked by a
// higher layer; fragmentation at this layer is reserved and unimplemented in v1.
inline constexpr std::size_t kMaxDatagramSize = 1232;

// Envelope flag bits (unscoped so bitwise ops with the u8 `flags` field are
// ergonomic). Undefined bits must be 0 in v1 and are ignored by receivers.
enum Flag : std::uint8_t {
    kFlagNone = 0x00,
    kFlagEncrypted = 0x01,    // payload is AEAD ciphertext (incl. auth tag)
    kFlagAckRequested = 0x02, // sender requests an ACK for this counter
    kFlagFragmented = 0x04,   // reserved; must be 0 in v1
};

// A parsed packet envelope. The variable fields are opaque bytes at this layer:
// `payload` is ciphertext when the ENCRYPTED flag is set. Routing servers may
// read `source`/`destination` but never `payload` for a MESSAGE (SRS §5/§6/§8).
struct Packet {
    std::uint8_t version = kProtocolVersion;
    PacketType type = PacketType::Hello;
    std::uint8_t flags = kFlagNone;
    std::uint64_t counter = 0; // per-sender monotonic: ordering + replay (§7)
    Bytes source;              // sender routing id / fingerprint (may be empty)
    Bytes destination;         // recipient routing id / fingerprint (may be empty)
    Bytes payload;             // opaque; ciphertext when kFlagEncrypted set

    [[nodiscard]] bool has_flag(Flag f) const {
        return (flags & static_cast<std::uint8_t>(f)) != 0;
    }
    [[nodiscard]] bool encrypted() const { return has_flag(kFlagEncrypted); }
};

// Result of parsing a datagram (see docs/protocol/wire-format.md §9).
enum class ParseError {
    Ok,                 // fully-consumed, well-formed packet
    TooShort,           // fewer than kHeaderSize bytes
    BadMagic,           // magic != kMagic
    UnsupportedVersion, // version != kProtocolVersion
    TruncatedField,     // a length-prefixed field runs past the buffer
    TrailingBytes,      // extra bytes remain after the payload
};

[[nodiscard]] std::string_view to_string(ParseError error);

// Serialize a packet to its v1 wire representation.
//
// Precondition: each of source/destination/payload is at most 65535 bytes
// (u16 length prefix). This holds for any datagram-sized packet.
[[nodiscard]] Bytes serialize(const Packet& packet);

// The bytes an AEAD binds as associated data: the fixed header plus the source
// and destination fields — everything except the payload (wire-format §8). Both
// peers compute this identically, so tampering with any envelope field makes
// decryption fail. Independent of the payload, so it can be computed before the
// ciphertext exists.
[[nodiscard]] Bytes associated_data(const Packet& packet);

// Parse a packet from wire bytes, reporting the precise failure reason.
[[nodiscard]] ParseError parse(const Bytes& bytes, Packet& out);

// Convenience wrapper: returns true iff parse() succeeded.
[[nodiscard]] bool deserialize(const Bytes& bytes, Packet& out);

} // namespace openmesh::protocol
