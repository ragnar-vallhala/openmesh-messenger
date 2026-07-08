# third_party

Vendored / external dependencies.

## libsodium

The cryptographic backend for `openmesh::crypto` (see
[docs/crypto/design.md](../docs/crypto/design.md) and
[ADR 0002](../docs/adr/0002-crypto-primitives.md)). The build consumes it through
the `sodium::sodium` CMake target, wired up by
[`cmake/Sodium.cmake`](../cmake/Sodium.cmake).

There are two supported ways to provide it:

### 1. System package (default)

`-DOPENMESH_VENDOR_SODIUM=OFF` (the default) uses the system libsodium, located by
[`cmake/FindSodium.cmake`](../cmake/FindSodium.cmake) via pkg-config.

```sh
sudo apt install libsodium-dev      # Debian/Ubuntu
brew install libsodium              # macOS
```

### 2. Vendored, pinned build

`-DOPENMESH_VENDOR_SODIUM=ON` builds a pinned libsodium instead — reproducible and
self-contained (useful for CI, Android, or hosts without the dev package). It
needs network access at configure time **unless** a submodule is present.

**FetchContent (no submodule):** the pinned version is fetched automatically. The
pin lives in `cmake/Sodium.cmake` (`GIT_TAG` / `OPENMESH_SODIUM_VERSION`); bump it
deliberately and record the change in an ADR.

**Git submodule (fully offline after checkout):** add a CMake-enabled libsodium
at `third_party/libsodium` and `cmake/Sodium.cmake` will build it directly:

```sh
git submodule add https://github.com/robinlinden/libsodium-cmake.git third_party/libsodium
cd third_party/libsodium && git checkout <pinned-tag> && git submodule update --init --recursive
```

Then configure with `-DOPENMESH_VENDOR_SODIUM=ON`.

> Do not commit build artifacts here. Submodule pointers (a pinned commit) are
> fine; unpacked source trees are not.
