# openmesh-signaling

A **volunteer signaling server** (SRS FR-6, §11). Anyone can host one; many
independent instances coexist with no central authority (SRS §1.2, Success
Criteria).

Responsibilities (SRS §11):

- Maintain a registry of currently active peers (keyed by public key / fingerprint).
- Answer `DISCOVER` lookups.
- Relay `CONNECT` / ICE connection information between peers (SRS FR-6).
- Expire and forget inactive peers automatically.
- Verify **proof of key ownership** on `REGISTER` — without ever holding a
  private key.

The server shall **never**:

- Store conversations or read messages (it only sees the packet envelope).
- Possess private keys or decrypt traffic (SRS FR-6).

## Run

```sh
openmesh-signaling --config servers/signaling/config/signaling.example.toml
```

Configuration lives in [`config/`](config/). Links against the shared `protocol`
and `crypto` libraries so it speaks the exact same wire format as clients.
