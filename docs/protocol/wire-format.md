# Wire Format & Packet Reference

> Status: **TODO** — this is the spec the `openmesh::protocol` library must
> implement. Nothing here is final.

## Transport

- UDP initially; QUIC later (SRS §8).

## Envelope (to be specified)

Every packet begins with a fixed envelope that servers may read for routing.
Message *contents* are ciphertext and are never readable by signaling or relay
servers (SRS §5, §6, §8).

Proposed fields (draft):

| Field         | Size | Notes                                            |
|---------------|------|--------------------------------------------------|
| `version`     | 1 B  | Protocol version (`kProtocolVersion`).           |
| `type`        | 1 B  | `PacketType` (see below).                        |
| `flags`       | 1 B  | Reserved.                                        |
| `length`      | 2 B  | Payload length.                                  |
| `payload`     | var  | Type-specific; ciphertext for `MESSAGE`.         |
| `auth_tag`    | var  | Packet authentication (SRS §5).                  |

Open questions: serialization (hand-rolled vs CBOR vs protobuf), endianness,
replay-protection counter placement, max packet size vs MTU.

## Packet types (SRS §9)

`HELLO`, `REGISTER`, `DISCOVER`, `CONNECT`, `CONTACT_REQUEST`,
`CONTACT_RESPONSE`, `MESSAGE`, `ACK`, `HEARTBEAT`, `DISCONNECT`.

Canonical numeric values live in
[`libs/protocol/include/openmesh/protocol/packet_type.hpp`](../../libs/protocol/include/openmesh/protocol/packet_type.hpp).
