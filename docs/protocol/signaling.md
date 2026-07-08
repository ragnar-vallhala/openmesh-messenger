# Signaling Sub-Protocol (v1)

Status: **v1 — implemented** in `servers/signaling` (`SignalingServer`). Describes
how peers use a volunteer signaling server to register, find each other, and
exchange connection info (SRS FR-6, FR-7, §11). Built on the
[wire format](wire-format.md); all packets are v1 envelopes.

The server reads only the envelope (type + routing ids + payload framing). It
never decrypts message contents and never holds a private key (SRS §5, §6, §11).

## Identities & keys

A peer is identified by its Ed25519 public key (32 bytes). Throughout, the
registry is keyed by the lowercase hex of that public key. Peers learn each
other's public keys out of band (that key *is* the identity — SRS §4).

## Registration — proof of ownership (SRS §11)

A challenge-response handshake proves the peer holds the private key **and** can
receive at the address the packet came from (return-routability). The server
registers the peer at its **observed source endpoint**, so a peer cannot register
someone else's key or point discovery at a third party.

```
client → REGISTER   src = public_key           payload = (empty)          "please register me"
server → HELLO       payload = challenge (32 random bytes)                 (bound to client's src endpoint)
client → REGISTER   src = public_key           payload = Ed25519 sig(challenge)
server → ACK         (on success; peer now in the registry)
```

Rules:
- The signed REGISTER must arrive from the **same endpoint** that received the
  challenge, else it is ignored.
- Challenges expire after `challenge_ttl_seconds` (default 30). A failed
  signature discards the pending challenge.
- Registrations expire after `peer_ttl_seconds` of inactivity (default 120);
  `HEARTBEAT` refreshes liveness.

## Discovery

```
client → DISCOVER    dst = target_public_key
server → CONNECT     src = target_public_key
                     payload = "host:port"  (target online)  |  empty (offline/unknown)
```

The returned `host:port` is the target's registered (observed) endpoint — the
starting point for a direct connection (SRS FR-7).

> **Shared transport.** Because the server registers a peer at the endpoint it is
> *observed* from, a peer must run signaling and messaging over the **same UDP
> socket** — otherwise the registered address would not be where messages arrive.
> In the implementation, `net::SignalingClient` borrows the messaging socket
> (`messaging::Engine::transport()`), so a discovered address reaches the peer's
> messenger directly. The `om-connect-demo` tool shows the full flow.

## Connection-info relay (NAT traversal)

To exchange ICE/connection candidates, a peer sends `CONNECT` addressed to
another peer; the server forwards it verbatim to that peer's registered endpoint
(it only routes on the envelope):

```
A → CONNECT   src = A_pub   dst = B_pub   payload = A's connection info
server → CONNECT (forwarded to B's endpoint, unchanged)
```

Both peers exchanging candidates this way can then attempt direct → NAT-traversed
connectivity, falling back to a relay (SRS FR-7, FR-8).

## Liveness & teardown

```
client → HEARTBEAT   src = public_key      → server refreshes last-seen, replies HEARTBEAT
client → DISCONNECT  src = public_key      → server removes the peer from the registry
```

## Not handled by the signaling server

`MESSAGE`, `ACK` (peer-to-peer delivery), and `CONTACT_REQUEST` /
`CONTACT_RESPONSE` are peer-to-peer and are ignored if sent to the server.

## Reference

- Implementation: `servers/signaling/src/server.cpp`
- Tests (handshake, discovery, relay, teardown, expiry, live socket handshake):
  `servers/signaling/tests/test_server.cpp`
