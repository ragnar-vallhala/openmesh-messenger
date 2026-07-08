# Cryptography Design (v1)

Status: **Locked v1 — primitives implemented.** The algorithm choices below are
decided (see [ADR 0002](../adr/0002-crypto-primitives.md)). The low-level
primitives — identity, signing, key agreement, and AEAD — are implemented in
`openmesh::crypto` on top of **libsodium**. The forward-secret session ratchet
(§6) is designed here but not yet implemented.

Any change to an algorithm or on-the-wire crypto parameter requires a new ADR.

## 1. Requirements traced (SRS §5, §6, §11, FR-1, FR-5)

- End-to-end encryption of every message; authenticate every packet; reject
  tampered packets.
- Replay protection.
- Perfect Forward Secrecy (target; §6).
- Secure on-device key storage.
- Proof of key ownership without revealing private keys (§11).

## 2. Backend

**libsodium** (see [`third_party/README.md`](../../third_party/README.md) for how
it is vendored/linked). Chosen for being audited, misuse-resistant, and portable
to Android. It is an implementation detail — no `<sodium.h>` type appears in any
public `openmesh::crypto` header, so the backend can be replaced without churning
call sites.

## 3. Primitives (decided)

| Purpose             | Algorithm                        | libsodium API                              |
|---------------------|----------------------------------|--------------------------------------------|
| Identity signing    | **Ed25519**                      | `crypto_sign_*`                            |
| Key agreement       | **X25519** (derived from Ed25519)| `crypto_scalarmult` + `..._to_curve25519`  |
| AEAD (messages)     | **XChaCha20-Poly1305-IETF**      | `crypto_aead_xchacha20poly1305_ietf_*`     |
| Hash / fingerprint  | **BLAKE2b-256**                  | `crypto_generichash`                       |
| Password KDF (§7)   | **Argon2id**                     | `crypto_pwhash`                            |
| Session ratchet (§6)| **Double Ratchet** (planned)     | built on the above                         |

Key/opaque sizes (mirrored as constants in `common.hpp`, static_assert-checked):
public key 32 B, secret key 64 B, signature 64 B, fingerprint 32 B, session key
32 B, AEAD nonce 24 B, AEAD tag 16 B.

## 4. Identity (SRS §4, FR-1)

- An identity is a single **Ed25519** key pair. The **public key** is the user's
  permanent identity; the **secret key** never leaves the device.
- **Fingerprint** = lowercase hex of `BLAKE2b-256(public_key)` — the
  human-shareable identifier. No usernames, emails or phone numbers.
- **Export/import**: the raw secret key round-trips an identity
  (`identity_from_secret` recovers the public key from it).
- A single Ed25519 identity is reused for X25519 key agreement via the standard
  `ed25519 → curve25519` conversion, avoiding a second long-term key.

## 5. Key agreement

Both peers derive an identical 32-byte session key from their identities:

1. Convert each side's Ed25519 keys to X25519.
2. `dh = X25519(my_curve_sk, their_curve_pk)` — fails closed on low-order points.
3. `session_key = BLAKE2b(dh ‖ lo_pub ‖ hi_pub)` where `lo_pub`/`hi_pub` are the
   two X25519 public keys in canonical (sorted) order, so initiator and
   responder compute the same key.

This static ECDH is the **building block**. It is not yet forward-secret on its
own — that comes from the ratchet (§6).

## 6. Sessions, PFS & replay (target)

- **Handshake:** an X3DH-style exchange over `HELLO`/`CONTACT_REQUEST` /
  `CONTACT_RESPONSE` to agree an initial root key.
- **Ratchet:** a **Double Ratchet** advances message keys per message, giving
  Perfect Forward Secrecy and post-compromise security (SRS §6, future).
- **Replay/ordering:** the wire `counter` (per-sender monotonic, see
  [wire-format §7](../protocol/wire-format.md#7-counter-ordering--replay-protection))
  is tracked per session; non-increasing counters are rejected.

## 7. Message encryption & envelope integrity

- Messages are sealed with **XChaCha20-Poly1305-IETF**. `seal()` returns
  `nonce ‖ ciphertext ‖ tag`; the 24-byte nonce is random per message (XChaCha's
  extended nonce makes random generation collision-safe).
- The **serialized packet envelope** (fixed header + `source` + `destination`) is
  passed as AEAD **associated data**, binding it to the ciphertext. Tampering
  with any envelope field makes `open()` fail — so tampered packets are rejected
  without a separate signature field (SRS §5; wire-format §8).

## 8. Proof of key ownership (SRS §11)

A signaling server issues a random challenge on `REGISTER`; the peer returns an
**Ed25519 signature** over it (`sign_detached` / `verify_detached`). The server
verifies against the claimed public key. It never sees or stores a private key.

## 9. Key storage (implemented)

The on-device secret key and contact database are encrypted at rest with a key
derived from a user passphrase via **Argon2id** (`crypto::derive_key`,
`crypto_pwhash`) and sealed with XChaCha20-Poly1305 (`openmesh::storage`, SRS
FR-9/§13). The key is derived once per session and reused for cheap saves.
Platform secure storage (desktop keyring; Android Keystore) to hold that key is a
follow-up.

## 10. Deferred / open items

- Double Ratchet implementation and the X3DH handshake message formats.
- Multi-device key sharing (SRS FR-10).
- At-rest key storage implementation per platform.
- Formal review of the key-agreement KDF against a standard (HKDF vs BLAKE2b).

## 11. Reference

- Public API: `libs/crypto/include/openmesh/crypto/`
  (`identity.hpp`, `sign.hpp`, `key_agreement.hpp`, `aead.hpp`).
- Tests exercising every primitive: `libs/crypto/tests/test_identity.cpp`.
