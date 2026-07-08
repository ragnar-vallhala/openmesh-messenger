# ADR 0001: Single repository with shared libraries

- Status: Accepted
- Date: 2026-07-09

## Context

OpenMesh Messenger has four deployable components (desktop client, Android
client, signaling server, relay server) that must all speak the same wire
protocol and agree on cryptographic handling (SRS §2, §5, §8). The desktop and
Android clients are explicitly one Qt codebase (SRS §1.2).

## Decision

Use a **single repository (monorepo)**. Put all code that is shared between
components in `libs/` as separate CMake libraries (`core`, `protocol`, `crypto`,
`net`, `storage`, `messaging`); give each deployable component its own top-level
folder (`client/`, `servers/signaling/`, `servers/relay/`, `tools/`).

The build is CMake-based with per-component toggles so a headless host can build
just the servers without Qt.

## Consequences

- The client and both servers link the *same* `protocol` and `crypto` code — they
  cannot drift out of sync.
- The Messaging Engine lives in `libs/messaging`, separate from `client/`, keeping
  it independent of the UI (SRS §10) and headlessly testable.
- One clone, one build graph, atomic cross-component changes.
- Trade-off: a single repo can grow large; mitigated by the component toggles and
  clear library boundaries.
