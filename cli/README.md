# om-chat — command-line client

A minimal terminal client for OpenMesh Messenger: register with a signaling
server, discover peers by public key, run the contact-request flow, and exchange
end-to-end encrypted messages.

Networking runs on a background thread that owns the messaging engine; the main
thread only reads stdin, so all engine access stays single-threaded.

## Build & run

```sh
cmake -S . -B build && cmake --build build --target om-chat
./build/cli/om-chat (--server <host:port> | --relay <host:port>) \
    [--identity <file> --pass <phrase>] [--name <display>]
```

- `--server` — address of a signaling server (`openmesh-signaling`), for direct
  connections. Optional if `--relay` is given.
- `--relay` — address of a relay server (`openmesh-relay`). **Works through NAT**:
  both peers only send outbound to the relay, which forwards by destination public
  key. In relay mode `/add` needs no discovery — just the peer's public key. One
  of `--server` / `--relay` is required (both may be given).
- `--identity <file>` + `--pass <phrase>` — persist a stable identity, encrypted
  at rest. Without them a fresh (ephemeral) identity is used each run. The
  passphrase may also be given via the `OM_PASS` environment variable.
- `--name` — a display name sent as the greeting on a contact request.

## Commands

| Command | Action |
|---|---|
| `/id` | Show your public key (share it out of band) and fingerprint |
| `/add <name> <pubkeyhex>` | Discover a peer by public key and send a contact request |
| `/accept <name>` / `/reject <name>` | Respond to an incoming contact request |
| `/msg <name> <text>` | Send a message to an accepted contact |
| `<text>` | Send to the current peer (the last one you used) |
| `/peers` | List known peers and trust status |
| `/register` | (Re)register with the signaling server |
| `/quit` | Exit |

## Trying it across the internet

1. **Run a signaling server on a public host** (a VPS with a public IP):
   ```sh
   ./openmesh-signaling 0.0.0.0 4433
   ```
   Open UDP port 4433 in the firewall.

2. **Each person runs the client**, pointing at that server:
   ```sh
   ./om-chat --server <public-ip>:4433 --name alice
   ```
   Run `/id` and share your public-key hex with the other person (any channel).

3. **Connect:** one side runs `/add bob <bob's pubkey>`; the other sees the
   request and runs `/accept <name>`. Then `/msg` (or just type) to chat.

### Reachability caveat (no relay yet)

The signaling server only tells peers each other's **observed** address. Actually
delivering packets works when the peers are mutually reachable:

- ✅ both on public IPs, or on the same LAN, or one side on a public IP;
- ❌ **both behind typical home/mobile NAT** — needs UDP hole-punching or a relay,
  which are not built yet (FR-8).

So for a first internet test, put at least one peer on a publicly reachable host
(e.g. run one `om-chat` on the same VPS as the signaling server).
