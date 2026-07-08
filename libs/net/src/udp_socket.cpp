#include "openmesh/net/udp_socket.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

// POSIX sockets backend (Linux/macOS/Android).
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace openmesh::net {
namespace {

// Resolve host+port into a sockaddr. `family` may be AF_UNSPEC to accept either
// IPv4 or IPv6; `passive` selects an address suitable for bind(). Returns false
// if resolution yields nothing.
bool resolve(const std::string& host, std::uint16_t port, int family, bool passive,
             sockaddr_storage& out, socklen_t& out_len) {
    addrinfo hints{};
    hints.ai_family = family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    if (passive) {
        hints.ai_flags |= AI_PASSIVE;
    }

    const char* node = host.empty() ? nullptr : host.c_str();
    const std::string service = std::to_string(port);

    addrinfo* result = nullptr;
    if (::getaddrinfo(node, service.c_str(), &hints, &result) != 0 || result == nullptr) {
        return false;
    }
    std::memcpy(&out, result->ai_addr, result->ai_addrlen);
    out_len = static_cast<socklen_t>(result->ai_addrlen);
    ::freeaddrinfo(result);
    return true;
}

// Convert a sockaddr back into an Endpoint (numeric host + port).
Endpoint endpoint_from_sockaddr(const sockaddr_storage& ss, socklen_t len) {
    char host[NI_MAXHOST] = {};
    char serv[NI_MAXSERV] = {};
    Endpoint ep;
    if (::getnameinfo(reinterpret_cast<const sockaddr*>(&ss), len, host, sizeof(host), serv,
                      sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
        ep.host = host;
        ep.port = static_cast<std::uint16_t>(std::strtoul(serv, nullptr, 10));
    }
    return ep;
}

} // namespace

UdpSocket::~UdpSocket() {
    close();
}

UdpSocket::UdpSocket(UdpSocket&& other) noexcept : fd_(other.fd_), family_(other.family_) {
    other.fd_ = -1;
    other.family_ = 0;
}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = other.fd_;
        family_ = other.family_;
        other.fd_ = -1;
        other.family_ = 0;
    }
    return *this;
}

void UdpSocket::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
        family_ = 0;
    }
}

bool UdpSocket::bind(const Endpoint& local) {
    close();

    sockaddr_storage ss{};
    socklen_t len = 0;
    if (!resolve(local.host, local.port, AF_UNSPEC, /*passive=*/true, ss, len)) {
        return false;
    }

    fd_ = ::socket(ss.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (fd_ < 0) {
        return false;
    }
    family_ = ss.ss_family;

    const int yes = 1;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (::bind(fd_, reinterpret_cast<const sockaddr*>(&ss), len) != 0) {
        close();
        return false;
    }
    return true;
}

bool UdpSocket::send_to(const Endpoint& to, const Bytes& data) {
    sockaddr_storage ss{};
    socklen_t len = 0;
    const int family = is_open() ? family_ : AF_UNSPEC;
    if (!resolve(to.host, to.port, family, /*passive=*/false, ss, len)) {
        return false;
    }

    if (!is_open()) {
        fd_ = ::socket(ss.ss_family, SOCK_DGRAM, IPPROTO_UDP);
        if (fd_ < 0) {
            return false;
        }
        family_ = ss.ss_family;
    }

    const ssize_t sent =
        ::sendto(fd_, data.data(), data.size(), 0, reinterpret_cast<const sockaddr*>(&ss), len);
    return sent >= 0 && static_cast<std::size_t>(sent) == data.size();
}

std::optional<Datagram> UdpSocket::receive(int timeout_ms, std::size_t max_size) {
    if (!is_open()) {
        return std::nullopt;
    }

    if (timeout_ms >= 0) {
        pollfd pfd{};
        pfd.fd = fd_;
        pfd.events = POLLIN;
        const int ready = ::poll(&pfd, 1, timeout_ms);
        if (ready <= 0) {
            return std::nullopt; // timeout or error
        }
    }

    Bytes buffer(max_size);
    sockaddr_storage ss{};
    socklen_t len = sizeof(ss);
    const ssize_t got =
        ::recvfrom(fd_, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(&ss), &len);
    if (got < 0) {
        return std::nullopt;
    }

    buffer.resize(static_cast<std::size_t>(got));
    Datagram dg;
    dg.data = std::move(buffer);
    dg.from = endpoint_from_sockaddr(ss, len);
    return dg;
}

bool UdpSocket::set_nonblocking(bool on) {
    if (!is_open()) {
        return false;
    }
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    flags = on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return ::fcntl(fd_, F_SETFL, flags) == 0;
}

std::optional<Endpoint> UdpSocket::local_endpoint() const {
    if (!is_open()) {
        return std::nullopt;
    }
    sockaddr_storage ss{};
    socklen_t len = sizeof(ss);
    if (::getsockname(fd_, reinterpret_cast<sockaddr*>(&ss), &len) != 0) {
        return std::nullopt;
    }
    return endpoint_from_sockaddr(ss, len);
}

} // namespace openmesh::net
