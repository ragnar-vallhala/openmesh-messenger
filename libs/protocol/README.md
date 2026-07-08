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
- Serialization format is TBD — see [`docs/protocol/`](../../docs/protocol/).
  Candidates: a compact hand-rolled binary layout, CBOR, or protobuf.
- Every packet carries authentication material so tampered packets are rejected
  (SRS §5); the actual MAC/signature lives in `openmesh::crypto`.
