#include "openmesh/protocol/packet.hpp"

#include <cassert>

namespace openmesh::protocol {
namespace {

// --- big-endian write helpers ------------------------------------------------

void put_u16(Bytes& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(value & 0xFF));
}

void put_u64(Bytes& out, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
    }
}

// Write a u16 length prefix followed by the field bytes.
void put_field(Bytes& out, const Bytes& field) {
    assert(field.size() <= 0xFFFF && "field exceeds u16 length prefix");
    put_u16(out, static_cast<std::uint16_t>(field.size()));
    out.insert(out.end(), field.begin(), field.end());
}

// --- big-endian read helpers -------------------------------------------------
// All readers assume the caller has already bounds-checked `offset`.

std::uint16_t get_u16(const Bytes& in, std::size_t offset) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(in[offset]) << 8) |
                                      static_cast<std::uint16_t>(in[offset + 1]));
}

std::uint64_t get_u64(const Bytes& in, std::size_t offset) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        value = (value << 8) | static_cast<std::uint64_t>(in[offset + i]);
    }
    return value;
}

// Read a u16-length-prefixed field, advancing `offset`. Returns false if the
// declared length runs past the end of the buffer.
bool read_field(const Bytes& in, std::size_t& offset, Bytes& out) {
    if (offset + sizeof(std::uint16_t) > in.size()) {
        return false;
    }
    const std::uint16_t len = get_u16(in, offset);
    offset += sizeof(std::uint16_t);
    if (offset + len > in.size()) {
        return false;
    }
    out.assign(in.begin() + static_cast<std::ptrdiff_t>(offset),
               in.begin() + static_cast<std::ptrdiff_t>(offset + len));
    offset += len;
    return true;
}

} // namespace

std::string_view to_string(ParseError error) {
    switch (error) {
    case ParseError::Ok:
        return "ok";
    case ParseError::TooShort:
        return "too-short";
    case ParseError::BadMagic:
        return "bad-magic";
    case ParseError::UnsupportedVersion:
        return "unsupported-version";
    case ParseError::TruncatedField:
        return "truncated-field";
    case ParseError::TrailingBytes:
        return "trailing-bytes";
    }
    return "unknown";
}

Bytes serialize(const Packet& packet) {
    Bytes out;
    out.reserve(kHeaderSize + 3 * sizeof(std::uint16_t) + packet.source.size() +
                packet.destination.size() + packet.payload.size());

    // Fixed header.
    put_u16(out, kMagic);
    out.push_back(packet.version);
    out.push_back(static_cast<std::uint8_t>(packet.type));
    out.push_back(packet.flags);
    put_u64(out, packet.counter);

    // Variable, length-prefixed sections.
    put_field(out, packet.source);
    put_field(out, packet.destination);
    put_field(out, packet.payload);

    return out;
}

ParseError parse(const Bytes& bytes, Packet& out) {
    if (bytes.size() < kHeaderSize) {
        return ParseError::TooShort;
    }

    std::size_t offset = 0;
    if (get_u16(bytes, offset) != kMagic) {
        return ParseError::BadMagic;
    }
    offset += sizeof(std::uint16_t);

    const std::uint8_t version = bytes[offset++];
    const auto type = static_cast<PacketType>(bytes[offset++]);
    const std::uint8_t flags = bytes[offset++];
    const std::uint64_t counter = get_u64(bytes, offset);
    offset += sizeof(std::uint64_t);

    if (version != kProtocolVersion) {
        return ParseError::UnsupportedVersion;
    }

    Bytes source, destination, payload;
    if (!read_field(bytes, offset, source) || !read_field(bytes, offset, destination) ||
        !read_field(bytes, offset, payload)) {
        return ParseError::TruncatedField;
    }

    if (offset != bytes.size()) {
        return ParseError::TrailingBytes;
    }

    out.version = version;
    out.type = type;
    out.flags = flags;
    out.counter = counter;
    out.source = std::move(source);
    out.destination = std::move(destination);
    out.payload = std::move(payload);
    return ParseError::Ok;
}

bool deserialize(const Bytes& bytes, Packet& out) {
    return parse(bytes, out) == ParseError::Ok;
}

} // namespace openmesh::protocol
