# om-chat — command-line client

A terminal client for OpenMesh Messenger. On a real terminal it runs a
**full-screen TUI** with a sidebar (Chats + Requests) and a conversation pane;
when output is piped/redirected it falls back to a simple line mode (handy for
scripting).

```
┌ OpenMesh  alice  8be636d097…            bob  ● direct ┐
│ CHATS               │ 12:20  bob   hey there          │
│▸bob      hey there  │ 12:21  you   hi bob              │
│                     │ 12:30  bob   how's it going      │
│ REQUESTS (1)        │                                  │
│ carol   "hi!"       │                                  │
├─────────────────────┴──────────────────────────────────┤
│ > type a message…                                      │
│ ↑/↓ select  Enter send  a/r accept/reject  /add /quit  │
└─────────────────────────────────────────────────────────┘
```

**Keys:** `↑`/`↓` select a chat or request · `Enter` send to the selected chat ·
`a`/`r` accept/reject the selected request · `PgUp`/`PgDn` scroll · commands
`/add <name> <pubkey>`, `/id`, `/accept`, `/reject`, `/quit`.

**Persistence:** with `--identity <file> --pass <phrase>`, your identity,
contacts, **conversation history**, and pending requests are stored encrypted at
rest (in `<file>.store`) and restored on the next launch.

Networking runs on a background thread that owns the messaging engine; the UI
thread only renders and reads keys, so engine access stays single-threaded.

## Build & run

```sh
cmake -S . -B build && cmake --build build --target om-chat
./build/cli/om-chat (--server <host:port> | --relay <host:port>) \
    [--identity <file> --pass <phrase>] [--name <display>]
```

- `--server` — a signaling server (`openmesh-signaling`): discovery + your public
  address (STUN) + hole-punch coordination.
- `--relay` — a relay server (`openmesh-relay`): forwards through NAT.
- One of the two is required. **Give both for the best experience** (recommended):
  messages flow via the relay immediately (reliable, NAT-proof) and the client
  hole-punches a **direct** path in the background, upgrading transparently when it
  succeeds — it prints `* direct connection to <peer> established (no relay)`. This
  is how WebRTC/Tailscale behave (TURN relay + ICE). `--relay` alone works but
  never goes direct; `--server` alone goes direct only when peers are reachable.
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
