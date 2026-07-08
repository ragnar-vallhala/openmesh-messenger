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

## Status

The **UDP packet loop is implemented** (`SignalingServer`): it binds a socket,
receives datagrams, and handles `REGISTER` (with the proof-of-ownership
challenge-response), `DISCOVER`, `CONNECT` relay, `HEARTBEAT`, and `DISCONNECT`
against the `PeerRegistry`, with peer/challenge expiry. See the
[signaling sub-protocol spec](../../docs/protocol/signaling.md).

The protocol logic (`handle()`) is separated from I/O so it is unit-tested
without sockets; there is also a live loopback-socket handshake test.

Still to come: TOML config parsing, structured metrics/logging, and abuse
throttling (SRS §7).

## Run

```sh
# bind_host defaults to 0.0.0.0, bind_port to 4433
openmesh-signaling 0.0.0.0 4433
```

Example configuration lives in [`config/`](config/) (not yet parsed — args are
positional for now). Links against the shared `protocol`, `net` and `crypto`
libraries so it speaks the exact same wire format as clients.
