# openmesh-relay

An **optional relay server** (SRS FR-8, §12). Used as the last resort in the
connection strategy when a direct connection and NAT traversal both fail
(SRS FR-7). Independently deployable — anyone can run one.

Responsibilities (SRS §12):

- Forward encrypted packets between two peers.
- Maintain only temporary routing state.
- Support many simultaneous clients.

Guarantees:

- **Content-agnostic** — stores no plaintext, cannot decrypt payloads (SRS §8,
  §13). It sees only the routing envelope, never message contents.

## Run

```sh
openmesh-relay --config servers/relay/config/relay.example.toml
```
