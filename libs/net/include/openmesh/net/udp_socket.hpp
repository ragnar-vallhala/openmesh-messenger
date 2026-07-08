#pragma once

#include "openmesh/net/endpoint.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace openmesh::net {

using Bytes = std::vector<std::uint8_t>;

// A received datagram together with the address it came from.
struct Datagram {
    Endpoint from;
    Bytes data;
};

// Default receive buffer size — comfortably larger than one UDP datagram
// (protocol::kMaxDatagramSize is 1232).
inline constexpr std::size_t kReceiveBufferSize = 2048;

// A UDP socket — the Socket Layer at the base of the Networking Engine
// (SRS §10). Thin, blocking-or-timeout wrapper over POSIX BSD sockets; supports
// IPv4 and IPv6. Deliberately Qt-free so the servers can use it too.
//
// Not thread-safe: a socket is intended to be driven by a single loop. A higher
// layer adds the direct → NAT traversal → relay strategy (SRS FR-7).
//
// NOTE: implemented for POSIX (Linux/macOS/Android). A Windows/Winsock backend
// is a TODO.
class UdpSocket {
public:
    UdpSocket() = default;
    ~UdpSocket();

    UdpSocket(UdpSocket&& other) noexcept;
    UdpSocket& operator=(UdpSocket&& other) noexcept;
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    // Open and bind to a local endpoint. `host` may be an address ("0.0.0.0",
    // "::", "127.0.0.1", ...); `port` 0 asks the OS for an ephemeral port
    // (query it afterwards via local_endpoint()). Returns false on failure.
    bool bind(const Endpoint& local);

    // Send `data` as a single datagram to `to`. If the socket is not yet open it
    // is opened with an ephemeral local port matching the destination's family.
    // Returns false if the whole datagram could not be sent.
    bool send_to(const Endpoint& to, const Bytes& data);

    // Receive one datagram. `timeout_ms` < 0 blocks indefinitely; >= 0 waits at
    // most that long and returns nullopt on timeout. Returns nullopt on error.
    std::optional<Datagram> receive(int timeout_ms = -1, std::size_t max_size = kReceiveBufferSize);

    // Switch between blocking and non-blocking I/O.
    bool set_nonblocking(bool on);

    // The socket's bound local address (resolves the actual ephemeral port).
    [[nodiscard]] std::optional<Endpoint> local_endpoint() const;

    [[nodiscard]] bool is_open() const { return fd_ >= 0; }
    [[nodiscard]] int native_handle() const { return fd_; }
    void close();

private:
    int fd_ = -1;
    int family_ = 0; // AF_INET / AF_INET6 once open
};

} // namespace openmesh::net
