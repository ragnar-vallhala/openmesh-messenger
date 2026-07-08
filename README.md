# OpenMesh Messenger

A decentralized, end-to-end encrypted messaging platform with no central authority.
Users own their cryptographic identities; anyone can run the signaling and relay
infrastructure. See [`SRS.md`](SRS.md) for the full requirements specification.

## Repository layout

This is a single repository (monorepo). Code shared between the client and the
servers lives in `libs/`; each deployable component lives in its own top-level
folder.

```
openmesh-messenger/
├── SRS.md                 # Software Requirements Specification
├── CMakeLists.txt         # Top-level build; toggles which parts to build
├── cmake/                 # Shared CMake helpers / toolchain files
├── docs/                  # Architecture, protocol and crypto design docs
│   ├── architecture/
│   ├── protocol/          # Wire format & packet-type reference
│   ├── crypto/            # Key/session/encryption design
│   └── adr/               # Architecture Decision Records
│
├── libs/                  # Shared, UI-independent libraries (client + servers)
│   ├── core/              # Common utilities: logging, config, ids, errors
│   ├── protocol/          # Packet types, serialization, wire format
│   ├── crypto/            # Crypto Engine: keys, sessions, encryption, auth
│   ├── net/               # Networking Engine: sockets, transport, NAT traversal
│   ├── messaging/         # Messaging Engine (UI-independent, per SRS §10)
│   └── storage/           # Local persistence: identity, contacts, conversations
│
├── client/                # Single Qt/QML app for Desktop + Android
│   ├── src/               # C++ application layer, models, QML bridge
│   ├── qml/               # QML UI (components + screens)
│   ├── resources/         # Icons, fonts, .qrc bundles
│   └── platform/          # Desktop- and Android-specific glue
│
├── servers/
│   ├── signaling/         # Volunteer signaling server (peer registry, discovery)
│   └── relay/             # Optional relay server (forwards encrypted packets)
│
├── cli/                   # om-chat — command-line client (register, discover, chat)
├── tools/                 # Developer/CLI tools (keygen, network simulator, demos)
├── scripts/               # Build, format and CI helper scripts
└── third_party/           # Vendored dependencies (git submodules or copies)
```

### Why `libs/` is shared

The signaling and relay servers speak the exact same wire protocol as the client,
and both must verify key ownership without ever decrypting messages. Defining
`protocol` and `crypto` **once** and linking them into every component guarantees
the client and servers can never drift out of sync.

The `messaging` library is deliberately separate from `client/` so the Messaging
Engine stays independent of the UI (SRS §10) and can be unit-tested headlessly.

## Building

Requires CMake ≥ 3.21, a C++20 compiler, and Qt 6 (for the client).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

Useful options (see `CMakeLists.txt`):

| Option                    | Default | Builds                                  |
|---------------------------|---------|-----------------------------------------|
| `OPENMESH_BUILD_CLIENT`   | `ON`    | The Qt/QML desktop & Android client     |
| `OPENMESH_BUILD_SERVERS`  | `ON`    | Signaling + relay servers               |
| `OPENMESH_BUILD_TOOLS`    | `ON`    | CLI developer tools                     |
| `OPENMESH_BUILD_TESTS`    | `ON`    | Unit tests for libs and components      |

To build only the servers (e.g. on a headless host without Qt):

```sh
cmake -S . -B build -DOPENMESH_BUILD_CLIENT=OFF -DOPENMESH_BUILD_TOOLS=OFF
cmake --build build -j
```

## Try it

```sh
# 1. run a signaling server (any host; public IP for internet use)
./build/servers/signaling/openmesh-signaling 0.0.0.0 4433
# 2. run the CLI client, share your /id, /add a peer, /msg them
./build/cli/om-chat --server <host>:4433 --name alice
```

See [`cli/README.md`](cli/README.md) for the client and [`deploy/README.md`](deploy/README.md)
for running the signaling server as a container (published to GHCR by CI) on
Docker or k3s.

## Status

Working v1 of the core: Ed25519 identities, a v1 wire protocol, libsodium crypto
(X25519 key agreement + XChaCha20-Poly1305), a UDP transport, a signaling server
with proof-of-ownership registration, the contact-request flow, end-to-end
encrypted messaging, at-rest encrypted persistence, and the `om-chat` CLI. Not yet
done: the relay server (FR-8), NAT hole-punching, and forward secrecy (Double
Ratchet). See `docs/` and per-directory READMEs.
