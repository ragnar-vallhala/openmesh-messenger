# ADR 0002: Cryptographic primitives and backend

- Status: Accepted
- Date: 2026-07-09
- Related: [ADR 0001](0001-monorepo-structure.md), [docs/crypto/design.md](../crypto/design.md)

## Context

The SRS mandates end-to-end encryption, packet authentication, replay protection,
proof of key ownership, and (future) Perfect Forward Secrecy (§5, §6, §11). None
of these named concrete algorithms or a library. We must not hand-roll crypto.

## Decision

Use **libsodium** as the cryptographic backend and lock the following primitives:

- **Ed25519** for identity signing and proof of key ownership.
- **X25519** for key agreement, derived from the Ed25519 identity key via the
  standard curve conversion (one long-term key pair per identity).
- **XChaCha20-Poly1305-IETF** as the AEAD for messages, with the packet envelope
  bound as associated data for integrity (no separate signature field).
- **BLAKE2b-256** for hashing and the identity fingerprint.
- **Argon2id** for at-rest key storage (deferred implementation).
- A **Double Ratchet** for forward-secret sessions (designed; deferred).

libsodium is consumed via the `sodium::sodium` CMake target. By default the build
uses the system libsodium (`cmake/FindSodium.cmake`); `-DOPENMESH_VENDOR_SODIUM=ON`
builds a pinned copy for reproducible/offline builds. `<sodium.h>` is kept out of
all public headers so the backend stays swappable.

## Consequences

- Real, audited primitives replace the earlier insecure placeholders; identity,
  signing, key agreement and AEAD are implemented and tested.
- One identity key (Ed25519) covers both signing and, by conversion, key
  agreement — less key management, one fingerprint.
- Binding the envelope as AEAD associated data means header tampering fails
  decryption, satisfying "reject tampered packets" without extra bytes on the wire.
- Forward secrecy is **not** yet delivered: the current key agreement is static
  ECDH. PFS depends on the deferred Double Ratchet — until then, sessions are
  encrypted and authenticated but not forward-secret. This is tracked in
  docs/crypto/design.md §6/§10.
- A dependency on libsodium (system or pinned) is now required to build the
  crypto library, servers, and client.
