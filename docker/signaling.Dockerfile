# Multi-stage build for the OpenMesh signaling server.
#
# Build only the signaling server (no Qt client, CLI, tools or tests) so the
# build image stays small and needs only libsodium + a C++ toolchain.

# ---- build stage ----------------------------------------------------------
FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        pkg-config \
        libsodium-dev \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
        -DOPENMESH_BUILD_CLIENT=OFF \
        -DOPENMESH_BUILD_CLI=OFF \
        -DOPENMESH_BUILD_TOOLS=OFF \
        -DOPENMESH_BUILD_TESTS=OFF \
    && cmake --build build --target openmesh-signaling -j"$(nproc)"

# ---- runtime stage --------------------------------------------------------
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        libsodium23 \
        libstdc++6 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --uid 10001 --no-create-home openmesh

COPY --from=build /src/build/servers/signaling/openmesh-signaling \
                  /usr/local/bin/openmesh-signaling

USER openmesh
EXPOSE 4433/udp

# Args are the bind host and port (see servers/signaling/src/main.cpp).
ENTRYPOINT ["/usr/local/bin/openmesh-signaling"]
CMD ["0.0.0.0", "4433"]
