# CLAUDE.md

Guidance for AI assistants (and humans) working in this repository.

## Project

**OpenMesh Messenger** — a decentralized, end-to-end encrypted messenger with no
central authority. The authoritative spec is [`SRS.md`](SRS.md); design docs live
in [`docs/`](docs/). Read the SRS before making non-trivial changes.

## Layout (see `README.md` for detail)

- `libs/` — shared libraries linked by the client **and** the servers:
  `core`, `protocol`, `crypto`, `net`, `storage`, `messaging`.
- `client/` — single Qt/QML app (Desktop + Android).
- `servers/signaling`, `servers/relay` — the two server components.
- `tools/` — CLI dev tools. `docs/` — architecture, protocol, crypto, ADRs.

## Build & test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Headless / no Qt: add `-DOPENMESH_BUILD_CLIENT=OFF -DOPENMESH_BUILD_TOOLS=OFF`.

Before committing non-trivial changes: the project **must configure, build, and
pass `ctest`**. Run `scripts/format.sh` (clang-format) on touched C/C++ files.

## Working rules

- **Follow the SRS.** New behaviour should trace to a requirement; if it doesn't,
  note it in `docs/` or an ADR first.
- **Shared before local.** Anything the client and servers both need (protocol,
  crypto, wire types) belongs in `libs/`, defined once — never duplicated.
- **Keep the Messaging Engine UI-independent** (SRS §10). No Qt types in
  `libs/messaging` or any other `libs/` code. Qt lives only in `client/`.
- **Servers never see plaintext.** Signaling and relay code must only touch the
  packet envelope, never message contents (SRS §5, §6, §8).
- **Crypto is not hand-rolled.** The current `libs/crypto` is an insecure
  placeholder; real primitives come from a vetted library (libsodium) per
  `docs/crypto/design.md`. Never present placeholder crypto as secure.
- **Wire values are stable.** Do not renumber existing `PacketType` values or
  break the envelope without a protocol-version bump and an ADR.
- **Tests travel with code.** Add/extend tests in the relevant `tests/` folder;
  keep them dependency-free (`<cassert>` + CTest) until a framework is vendored.
- **Never commit secrets.** No private keys, identities, or keystores
  (`.gitignore` blocks `*.key`, `*.pem`, `identity_*.json`).
- **Match surrounding style** — naming, comment density, and idiom of nearby code.

## Commit rules

- **Do NOT add any AI/Claude attribution to commits or PRs.** No
  `Co-Authored-By: Claude ...` line and no "Generated with Claude Code" trailer.
  Commit messages must read as ordinary human commits.
- Write clear, imperative subject lines (e.g. "Add relay routing table").
- Keep commits focused; don't mix unrelated changes.
- Do not commit build output (`build/`) or editor/tooling caches.
