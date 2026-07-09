# openmesh-relay

An **optional relay server** (SRS FR-8, §12). Used as the last resort in the
connection strategy when a direct connection and NAT traversal both fail
(SRS FR-7). Independently deployable — anyone can run one.

Responsibilities (SRS §12):

- Forward encrypted packets between two peers.
- Maintain only temporary routing state.
- Support many simultaneous clients.

Guarantees:

- **Content-agnostic** — stores no plaintext, cannot decrypt payloads (SRS §8,
  §13). It sees only the routing envelope, never message contents.

## Status — implemented

`RelayServer` binds a UDP socket and:

- records `source public key → observed endpoint` from every packet (learning
  where each peer is reachable; peers keep it fresh with periodic **announces**);
- forwards any packet carrying a `destination` to that peer's endpoint, **verbatim**
  (the encrypted payload is never inspected);
- expires idle routes on a TTL.

Because every peer only ever sends **outbound** to the relay, NAT never blocks it
— two peers both behind NAT can exchange messages through the relay. The protocol
logic (`handle()`) is I/O-free and unit-tested; there is also a full end-to-end
test (two `messaging::Engine`s exchanging encrypted messages via a real relay).

Clients use it with `om-chat --relay <host:port>`.

> v1 caveat: relay routes are unauthenticated (a peer is trusted to announce its
> own key). Since payloads are end-to-end encrypted, a hijacked route can only
> deny delivery, not read messages. A signed announce is a hardening follow-up.

## Run

```sh
# bind_host defaults to 0.0.0.0, bind_port to 4434
openmesh-relay 0.0.0.0 4434
```
