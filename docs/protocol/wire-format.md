# Wire Format & Packet Reference (v1)

Status: **v1 draft — implemented.** The framing described here is implemented by
the `openmesh::protocol` library. Per-type *payload* contents are still being
specified (marked "forthcoming" below); the envelope framing is stable.

Any incompatible change to the envelope requires a `version` bump and an ADR
(see [CLAUDE.md](../../CLAUDE.md)).

## 1. Scope

This document defines how a packet is laid out as bytes on the wire. It covers
the **envelope** (framing + routing) only. It deliberately does **not** define
cryptographic operations — those live in `openmesh::crypto` and
[`docs/crypto/design.md`](../crypto/design.md). At this layer the payload is an
opaque byte string; for encrypted packets it is AEAD ciphertext produced by the
crypto layer.

## 2. Transport

- **UDP** initially; QUIC later (SRS §8). Each packet is one datagram.
- **Recommended maximum datagram size: 1232 bytes** (IPv6 minimum MTU 1280 minus
  IPv6 + UDP headers) to avoid IP fragmentation. Exposed as
  `openmesh::protocol::kMaxDatagramSize`. Messages larger than one datagram must
  be chunked by a higher layer (fragmentation is reserved — see flags — and not
  implemented in v1).

## 3. Conventions

- **Endianness:** all multi-byte integers are **big-endian** (network byte order).
- **Field sizes:** `u8` = 1 byte, `u16` = 2 bytes, `u64` = 8 bytes.
- Variable-length fields are **length-prefixed** with a `u16` length, so a field
  can hold 0–65535 bytes.

## 4. Packet layout

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          magic = 0x4F4D       |    version    |     type      |   Fixed
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   header
|     flags     |                                               |  (13 bytes)
+-+-+-+-+-+-+-+-+                                               |
|                          counter (u64)                        |
+                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   src_len (u16)   |   source (src_len bytes) ...               |  Routing +
+-+-+-+-+-+-+-+-+-+-+                                            |  payload
|   dst_len (u16)   |   destination (dst_len bytes) ...          |  (variable)
+-+-+-+-+-+-+-+-+-+-+                                            |
|   pay_len (u16)   |   payload (pay_len bytes) ...              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 4.1 Fixed header (13 bytes)

| Offset | Size | Field     | Description                                             |
|-------:|-----:|-----------|---------------------------------------------------------|
| 0      | u16  | `magic`   | Constant `0x4F4D` (ASCII "OM"). Cheap noise/scan filter. |
| 2      | u8   | `version` | Protocol version. This document is version **1**.       |
| 3      | u8   | `type`    | `PacketType` (§6).                                      |
| 4      | u8   | `flags`   | Bit flags (§5).                                        |
| 5      | u64  | `counter` | Per-sender monotonic counter (§7): ordering + replay.   |

### 4.2 Variable sections (in order)

| Field         | Size          | Description                                             |
|---------------|---------------|--------------------------------------------------------|
| `src_len`     | u16           | Length of `source`.                                    |
| `source`      | `src_len` B   | Source routing id — sender fingerprint / route token.  |
| `dst_len`     | u16           | Length of `destination`.                               |
| `destination` | `dst_len` B   | Destination routing id — recipient fingerprint / token.|
| `pay_len`     | u16           | Length of `payload`.                                   |
| `payload`     | `pay_len` B   | Opaque bytes; AEAD ciphertext when `ENCRYPTED` is set. |

All three length fields are **always present**, even when zero. Minimum packet
size is therefore `13 + 2 + 2 + 2 = 19` bytes.

`source` / `destination` are opaque routing identifiers. A signaling or relay
server may read them to route a packet but must never be able to read `payload`
for a `MESSAGE` (SRS §5, §6, §8). To avoid leaking long-term identity to relays,
these may be ephemeral route tokens rather than raw public keys — that policy is
defined by the crypto/session layer, not here.

## 5. Flags

`flags` is a bitfield. Undefined bits MUST be 0 in v1 and MUST be ignored by
receivers (reserved for future use).

| Bit  | Name             | Meaning                                                    |
|------|------------------|------------------------------------------------------------|
| 0x01 | `ENCRYPTED`      | `payload` is AEAD ciphertext (incl. auth tag).             |
| 0x02 | `ACK_REQUESTED`  | Sender requests an `ACK` for this `counter`.               |
| 0x04 | `FRAGMENTED`     | Reserved for future chunking. MUST be 0 in v1.             |

## 6. Packet types

Numeric values are fixed on the wire and defined in
[`packet_type.hpp`](../../libs/protocol/include/openmesh/protocol/packet_type.hpp).
Do not renumber existing values.

| Value | Type              | Typical src → dst              | Payload (forthcoming unless noted)                    |
|-------|-------------------|--------------------------------|-------------------------------------------------------|
| 0x01  | `HELLO`           | peer ↔ peer / peer ↔ server    | Capability/version negotiation blob.                  |
| 0x02  | `REGISTER`        | peer → signaling               | Proof of key ownership (signed challenge/nonce, §11). |
| 0x03  | `DISCOVER`        | peer → signaling               | `destination` = looked-up fingerprint; payload empty. |
| 0x04  | `CONNECT`         | peer ↔ signaling ↔ peer        | Connection/ICE descriptor (candidates).               |
| 0x05  | `CONTACT_REQUEST` | peer → peer                    | One-shot request (display name, greeting), encrypted. |
| 0x06  | `CONTACT_RESPONSE`| peer → peer                    | Accept/reject decision + initial session material.    |
| 0x07  | `MESSAGE`         | peer → peer                    | AEAD ciphertext; `ENCRYPTED` set; `counter` orders it.|
| 0x08  | `ACK`             | peer → peer                    | `u64` acknowledged counter (big-endian).              |
| 0x09  | `HEARTBEAT`       | peer ↔ peer / peer ↔ server    | Empty. Keepalive / liveness.                          |
| 0x0A  | `DISCONNECT`      | peer → peer / peer → server    | Optional `u8` reason code.                            |

## 7. Counter, ordering & replay protection

- `counter` is a **per-sender, monotonically increasing** `u64`.
- Receivers track the highest counter seen **per session** and MUST reject a
  packet whose counter is not greater than the last accepted one, giving replay
  protection (SRS §6) and a basis for message ordering (SRS FR-4).
- `ACK` correlates to a specific `counter` (carried in its payload).
- A fresh session starts its counter at an agreed base (defined by the session
  handshake in the crypto layer).

## 8. Integrity & authentication

The framing layer does not compute or verify cryptographic tags. Instead:

- For encrypted packets, the crypto layer MUST bind the **serialized fixed
  header + `source` + `destination`** as AEAD **associated data (AAD)**. Any
  tampering with the envelope then causes decryption/authentication to fail, so
  tampered packets are rejected (SRS §5) without a separate signature field.
- Control packets that a server must authenticate (e.g. `REGISTER` proof of
  ownership) carry their signature/proof **inside `payload`**.

## 9. Parsing rules (validation)

A deserializer MUST reject a datagram when:

1. It is shorter than the 13-byte fixed header → `TooShort`.
2. `magic` ≠ `0x4F4D` → `BadMagic`.
3. `version` is unsupported → `UnsupportedVersion`.
4. Any length-prefixed field claims more bytes than remain in the datagram →
   `TruncatedField`. (This bounds all allocations by the input size.)
5. Bytes remain after the payload → `TrailingBytes`. v1 is strict; forward-compatible
   extensions are introduced via a `version` bump, not trailing data.

Only a fully-consumed, well-formed datagram yields a `Packet`.

## 10. Worked example

A minimal `HEARTBEAT` (type `0x09`) with empty src/dst/payload and `counter = 1`:

```
4F 4D            magic  = "OM"
01               version = 1
09               type   = HEARTBEAT
00               flags  = 0
00 00 00 00 00 00 00 01  counter = 1
00 00            src_len = 0
00 00            dst_len = 0
00 00            pay_len = 0
```

Total: 19 bytes. Round-tripping any `Packet` through `serialize` then
`deserialize` reproduces it exactly (see `libs/protocol/tests`).

## 11. Reference

- Types: `libs/protocol/include/openmesh/protocol/packet_type.hpp`
- Envelope + (de)serialization: `libs/protocol/include/openmesh/protocol/packet.hpp`
