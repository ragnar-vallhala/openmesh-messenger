# openmesh::storage

Local, device-owned persistence (SRS FR-9, §13). No cloud account is required;
the user owns all of their data.

## Implemented

Persistence is **encrypted at rest** (SRS §13): data is serialized and sealed
with XChaCha20-Poly1305 under a key derived from a user passphrase via **Argon2id**
(`crypto::derive_key`). Files are `[magic][version][salt][ nonce‖ciphertext‖tag ]`,
with the header bound as AEAD associated data and written atomically. Wrong
passphrase or a tampered file fails authentication.

- **`Store`** (`store.hpp`) — the trusted contact database (SRS FR-3). In-memory
  by default; call `open(path, passphrase)` to make it durable, after which
  mutations auto-persist. The key is derived once at `open()` so saves stay cheap.
- **Identity** (`identity_store.hpp`) — `save_identity()` / `load_identity()` keep
  the private key encrypted on device (SRS §9, FR-1).

See the `om-persist-demo` tool for a runnable demonstration.

## Still to come

- **Conversations** — message history with timestamps, delivery status and
  ordering (SRS FR-4) is not persisted yet.
- **Preferences** — client settings.
- A **SQLite** backend could replace the file format behind the same facade for
  larger histories and queryability; platform secure key storage (desktop
  keyring / Android Keystore) can hold the passphrase-derived key.
