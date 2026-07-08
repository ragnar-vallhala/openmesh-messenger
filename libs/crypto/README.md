# openmesh::crypto

The Crypto Engine (SRS §5, §10). Owns identity keys, session establishment,
message encryption/authentication, and proof of key ownership. UI- and
transport-independent so it can be reused by the client and reasoned about in
isolation.

Responsibilities:

- **Identity** — generate/import/export long-term key pairs; derive the public
  fingerprint that *is* the user's identity (SRS §4, FR-1).
- **Sessions** — establish authenticated, encrypted sessions between two peers
  (SRS FR-5); target Perfect Forward Secrecy (SRS §6, future).
- **Message crypto** — encrypt every message, authenticate every packet, reject
  tampered/replayed packets (SRS FR-5, §6).
- **Proof of ownership** — challenge/response so servers can verify a peer holds
  the private key without ever seeing it (SRS §11).

> ⚠️ **Do not hand-roll primitives.** The current sources are *placeholders that
> are NOT secure*. Before any real use, back this with a vetted library —
> **libsodium** is the recommended default:
> - Identity keys: Ed25519 (sign) + X25519 (key agreement)
> - AEAD: XChaCha20-Poly1305
> - Session ratchet / PFS: a Double Ratchet-style scheme
>
> The chosen algorithms and handshake are specified in
> [`docs/crypto/`](../../docs/crypto/).
