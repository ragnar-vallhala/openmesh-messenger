# openmesh::protocol

The wire format. Defines every packet that travels between clients, signaling
servers and relays, plus their (de)serialization. Shared by the client and both
servers so the protocol is defined exactly once.

Packet types (SRS §9):

| Type              | Direction                     | Purpose                                  |
|-------------------|-------------------------------|------------------------------------------|
| `HELLO`           | peer ↔ server / peer ↔ peer   | Initial greeting / capability exchange   |
| `REGISTER`        | peer → signaling              | Announce presence + proof of key ownership |
| `DISCOVER`        | peer → signaling              | Look up a peer by public key / fingerprint |
| `CONNECT`         | peer ↔ signaling              | Exchange connection (ICE) information    |
| `CONTACT_REQUEST` | peer → peer                   | One-shot request to become a contact     |
| `CONTACT_RESPONSE`| peer → peer                   | Accept / reject a contact request        |
| `MESSAGE`         | peer → peer                   | Encrypted chat payload                   |
| `ACK`             | peer → peer                   | Delivery acknowledgement / ordering      |
| `HEARTBEAT`       | peer ↔ peer / peer ↔ server   | Keepalive & liveness                     |
| `DISCONNECT`      | peer → peer / peer → server   | Graceful teardown                        |

Design notes:
- Servers parse only the **envelope** (routing headers); the `MESSAGE` payload is
  ciphertext and must never be decryptable by signaling or relay (SRS §5, §6, §8).
- The **v1 wire format is implemented**: a 13-byte fixed header
  (`magic`, `version`, `type`, `flags`, `counter`) followed by three
  u16-length-prefixed fields (`source`, `destination`, `payload`), big-endian.
  See [`docs/protocol/wire-format.md`](../../docs/protocol/wire-format.md) for the
  authoritative spec. `serialize()` / `parse()` / `deserialize()` live in
  [`packet.hpp`](include/openmesh/protocol/packet.hpp).
- Envelope integrity is achieved by binding the header + routing ids as AEAD
  associated data in `openmesh::crypto` (not by a separate signature field), so
  tampered packets fail to decrypt (SRS §5). Per-type payload formats are still
  being specified.
