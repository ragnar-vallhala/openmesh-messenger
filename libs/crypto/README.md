# openmesh::crypto

The Crypto Engine (SRS §5, §10). Owns identity keys, key agreement, message
encryption/authentication, and proof of key ownership. Backed by **libsodium**;
transport- and UI-independent. `<sodium.h>` is kept out of the public headers so
the backend stays swappable — see [ADR 0002](../../docs/adr/0002-crypto-primitives.md)
and [docs/crypto/design.md](../../docs/crypto/design.md) for the locked design.

## Public API

| Header               | What it provides                                                     |
|----------------------|---------------------------------------------------------------------|
| `identity.hpp`       | `Identity`, `generate_identity()`, `identity_from_secret()`, fingerprint (Ed25519 + BLAKE2b-256). |
| `sign.hpp`           | `sign_detached()` / `verify_detached()` — Ed25519 (proof of ownership, SRS §11). |
| `key_agreement.hpp`  | `agree_shared_key()` — X25519 ECDH → shared session key.            |
| `aead.hpp`           | `seal()` / `open()` — XChaCha20-Poly1305 AEAD with envelope as AAD. |
| `common.hpp`         | `Bytes` and the key/nonce/tag size constants.                       |

## Primitives (decided — ADR 0002)

- Identity signing: **Ed25519**
- Key agreement: **X25519** (derived from the Ed25519 identity key)
- Message AEAD: **XChaCha20-Poly1305-IETF** (envelope bound as associated data)
- Hash / fingerprint: **BLAKE2b-256**

## Status

Primitives are **implemented and tested** (identity, signing, key agreement,
AEAD). Still to come per the design doc: the X3DH-style handshake and **Double
Ratchet** for Perfect Forward Secrecy (SRS §6), and at-rest key storage
(Argon2id). Until the ratchet lands, sessions are encrypted and authenticated but
**not yet forward-secret** — do not describe them as PFS.
