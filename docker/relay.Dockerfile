# Multi-stage build for the OpenMesh relay server.
#
# The relay is content-agnostic and crypto-free, so its runtime image needs only
# the C++ standard library. (The build still needs libsodium-dev because the
# top-level CMake configure links the crypto library for other components.)

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
    && cmake --build build --target openmesh-relay -j"$(nproc)"

# ---- runtime stage --------------------------------------------------------
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        libstdc++6 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --uid 10001 --no-create-home openmesh

COPY --from=build /src/build/servers/relay/openmesh-relay /usr/local/bin/openmesh-relay

USER openmesh
EXPOSE 4434/udp

# Args are the bind host and port (see servers/relay/src/main.cpp).
ENTRYPOINT ["/usr/local/bin/openmesh-relay"]
CMD ["0.0.0.0", "4434"]
