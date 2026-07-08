# Cryptography Design

> Status: **TODO** — this is where the concrete scheme must be pinned down before
> any real use. The `openmesh::crypto` library currently ships insecure
> placeholders.

## Requirements (SRS §5, §6)

- End-to-end encryption of every message.
- Authentication of every packet; reject tampered packets.
- Replay protection.
- Perfect Forward Secrecy (future).
- Secure on-device key storage.
- Servers can verify **proof of key ownership** without seeing private keys (§11).

## Proposed primitives (to confirm)

Recommended backend: **libsodium**.

| Purpose            | Candidate                                   |
|--------------------|---------------------------------------------|
| Identity signing   | Ed25519                                     |
| Key agreement      | X25519                                      |
| AEAD (messages)    | XChaCha20-Poly1305                          |
| Hashing/fingerprint| BLAKE2b or SHA-256                          |
| Session / PFS      | Double Ratchet-style scheme                 |
| Proof of ownership | Sign a server-issued challenge (Ed25519)    |

## Identity (SRS §4, FR-1)

The public key (or its fingerprint) *is* the user's permanent identity. No
usernames, emails, or phone numbers. Private keys never leave the device.

## Open questions

- Exact handshake / session-setup message flow.
- Ratchet parameters and key rotation policy.
- Multi-device key sharing (SRS FR-10, future).
- Secure key storage per platform (desktop keyring vs Android Keystore).
