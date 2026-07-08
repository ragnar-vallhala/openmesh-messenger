#include "openmesh/protocol/packet.hpp"

#include <cassert>

using namespace openmesh::protocol;

static void test_full_roundtrip() {
    Packet in;
    in.type = PacketType::Message;
    in.flags = kFlagEncrypted | kFlagAckRequested;
    in.counter = 0x0102030405060708ULL;
    in.source = {0xAA, 0xBB};
    in.destination = {0xCC, 0xDD, 0xEE};
    in.payload = {0xDE, 0xAD, 0xBE, 0xEF};

    const Bytes bytes = serialize(in);

    Packet out;
    assert(parse(bytes, out) == ParseError::Ok);
    assert(out.version == kProtocolVersion);
    assert(out.type == PacketType::Message);
    assert(out.flags == in.flags);
    assert(out.counter == in.counter);
    assert(out.source == in.source);
    assert(out.destination == in.destination);
    assert(out.payload == in.payload);
    assert(out.encrypted());
    assert(out.has_flag(kFlagAckRequested));
    assert(!out.has_flag(kFlagFragmented));
}

static void test_empty_fields() {
    Packet in;
    in.type = PacketType::Heartbeat;
    in.counter = 1;

    const Bytes bytes = serialize(in);
    assert(bytes.size() == kMinPacketSize); // 19 bytes

    Packet out;
    assert(deserialize(bytes, out));
    assert(out.type == PacketType::Heartbeat);
    assert(out.source.empty() && out.destination.empty() && out.payload.empty());
}

static void test_byte_layout() {
    // Matches the worked example in docs/protocol/wire-format.md §10.
    Packet in;
    in.type = PacketType::Heartbeat; // 0x09
    in.counter = 1;

    const Bytes b = serialize(in);
    const Bytes expected = {0x4F, 0x4D,             // magic "OM"
                            0x01,                   // version
                            0x09,                   // type = HEARTBEAT
                            0x00,                   // flags
                            0x00, 0x00, 0x00, 0x00, // counter (big-endian) ...
                            0x00, 0x00, 0x00, 0x01, // ... = 1
                            0x00, 0x00,             // src_len
                            0x00, 0x00,             // dst_len
                            0x00, 0x00};            // pay_len
    assert(b == expected);
}

static void test_rejects_bad_input() {
    Packet out;

    // Too short.
    assert(parse({0x4F, 0x4D, 0x01}, out) == ParseError::TooShort);

    // Bad magic (13 bytes, wrong first two).
    assert(parse(Bytes(kHeaderSize, 0x00), out) == ParseError::BadMagic);

    // Unsupported version.
    Bytes wrong_ver = serialize(Packet{});
    wrong_ver[2] = 0xFF;
    assert(parse(wrong_ver, out) == ParseError::UnsupportedVersion);

    // Truncated field: claim a 4-byte source but provide none.
    Packet p;
    p.type = PacketType::Hello;
    Bytes truncated = serialize(p);
    truncated[kHeaderSize] = 0x00;     // src_len high byte
    truncated[kHeaderSize + 1] = 0x04; // src_len low byte = 4, but no bytes follow
    assert(parse(truncated, out) == ParseError::TruncatedField);

    // Trailing bytes after a valid packet.
    Bytes trailing = serialize(Packet{});
    trailing.push_back(0x99);
    assert(parse(trailing, out) == ParseError::TrailingBytes);
}

int main() {
    test_full_roundtrip();
    test_empty_fields();
    test_byte_layout();
    test_rejects_bad_input();

    assert(to_string(PacketType::ContactRequest) == "CONTACT_REQUEST");
    assert(to_string(ParseError::BadMagic) == "bad-magic");
    return 0;
}
