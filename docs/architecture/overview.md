# Architecture Overview

> Status: draft skeleton. Fill in as components are implemented.

## Components (SRS §2)

- **Desktop Client** and **Android Client** — one Qt/QML codebase (`client/`).
- **Signaling Server** — `servers/signaling/`.
- **Relay Server** (optional) — `servers/relay/`.

No component stores plaintext messages (SRS §2).

## Client layers (SRS §10)

```
QML UI            (client/qml)
   │
Application layer  (client/src: bridge, models)
   │
Messaging Engine   (libs/messaging)   ← UI-independent
   │
Crypto Engine      (libs/crypto)
   │
Networking Engine  (libs/net)
   │
Socket Layer       (libs/net)
```

Local persistence (`libs/storage`) hangs off the Messaging Engine.

## Why shared libraries

`protocol` and `crypto` are compiled once and linked into the client and both
servers, guaranteeing a single source of truth for the wire format and key
handling. See [ADR 0001](../adr/0001-monorepo-structure.md).

## Data flow: sending a message (target)

1. UI calls the bridge controller.
2. Messaging Engine looks up / establishes an encrypted session (crypto).
3. Engine builds and encrypts a `MESSAGE` packet (protocol + crypto).
4. Networking Engine sends it: direct → NAT traversal → relay (net, SRS FR-7).
5. Engine persists the message and tracks delivery (storage).
