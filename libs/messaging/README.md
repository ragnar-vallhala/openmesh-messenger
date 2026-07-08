# openmesh::messaging

The **Messaging Engine** (SRS §10). Orchestrates crypto, networking and storage
to deliver the application's behaviour — and is deliberately **independent of the
UI** so it can be driven by the desktop app, the Android app, or a headless test
harness identically (SRS §10: "The Messaging Engine shall remain independent of
the UI").

## Implemented

- **`Session`** (`session.hpp`) — an end-to-end encrypted session with one peer.
  Derives a shared key via X25519 key agreement, then `encrypt()`/`decrypt()`
  MESSAGE packets with XChaCha20-Poly1305, binding the packet envelope as AEAD
  associated data. A per-session monotonic counter gives message ordering and
  replay protection (rejects tampered, replayed, old, or misaddressed packets).
- **`Engine`** (`engine.hpp`) — the UI-independent Messaging Engine (SRS §10).
  Owns a UDP socket, a `Session` per peer, and the local contact database
  (`storage::Store`): `add_peer()`, `send()` (encrypt + transmit), and `poll()`
  (receive + decrypt + dispatch). No Qt types.
- **Contact-request flow** (SRS FR-2, §7). `send_contact_request()`,
  `accept_contact()`, `reject_contact()`, with `on_contact_request` /
  `on_contact_response` callbacks. Enforced rules: unknown users may send exactly
  **one** contact request; only **accepted** contacts can exchange messages —
  messages from non-accepted peers are dropped at the receiver. Contact
  requests/responses are themselves encrypted via the peer `Session`
  (`CONTACT_REQUEST` / `CONTACT_RESPONSE` packets).

See `om-chat-demo` (encryption) and `om-contacts-demo` (contact flow) for runnable
demonstrations, and `docs/crypto/design.md` for the session design.

> Not yet forward-secret: sessions use static ECDH; the Double Ratchet is a
> follow-up. Also still to come: durable persistence (the contact database is
> in-memory), timestamps/delivery status, and a non-blocking receive path.

## Responsibilities (target)

- Drive contact requests: the one-request rule and accept/reject/ignore (SRS FR-2).
- Send/receive messages with timestamps, delivery status, ordering and optional
  read receipts (SRS FR-4).
- Establish encrypted sessions via `crypto`, ship packets via `net`, and persist
  via `storage`.
- Expose an event/callback surface the UI subscribes to (no Qt types leak in here).

This is the top of the shared-library stack: it depends on `core`, `protocol`,
`crypto`, `net` and `storage`, and is consumed by `client/`.
