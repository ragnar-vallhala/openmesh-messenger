# openmesh::net

The Networking Engine and Socket Layer (SRS §10). Moves protocol packets between
peers and servers and implements the connection strategy — direct, NAT
traversal, then relay fallback (SRS FR-7).

Responsibilities:

- **Transport** — UDP initially; QUIC later (SRS §8).
- **Connection establishment** — direct → NAT traversal → relay fallback
  (SRS FR-7), with automatic reconnection and retransmission (SRS §6 Reliability).
- **Signaling client** — talk to signaling servers to register and discover
  peers and exchange ICE/connection info (SRS FR-6).
- **Relay client** — tunnel encrypted packets through a relay when direct
  connectivity fails (SRS FR-8).

Depends on `protocol` for packet (de)serialization. Knows nothing about message
contents — it only sees encrypted payloads.

## Socket Layer

`udp_socket.hpp` — **implemented.** `UdpSocket` is a thin, Qt-free wrapper over
POSIX BSD sockets (IPv4 + IPv6): `bind()`, `send_to()`, and `receive()` with an
optional timeout, plus non-blocking mode and `local_endpoint()`. `Datagram` pairs
received bytes with their source `Endpoint`. This is the base the higher-level
engine builds on. Windows/Winsock support is a TODO.

Still to come: the connection strategy (direct/NAT/relay), signaling- and
relay-client logic, reconnection, and retransmission — layered on top of this
socket.
