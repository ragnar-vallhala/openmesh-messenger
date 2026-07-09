#include "openmesh/net/signaling_client.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <utility>

namespace openmesh::net {
namespace {

using protocol::Packet;
using protocol::PacketType;

// Parse a "host:port" endpoint string as sent by the signaling server.
// NOTE: uses the last ':' as the separator; bracketless IPv6 hosts are a known
// limitation (matches Endpoint::to_string()).
std::optional<Endpoint> parse_endpoint(const Bytes& payload) {
    if (payload.empty()) {
        return std::nullopt;
    }
    const std::string s(payload.begin(), payload.end());
    const auto pos = s.rfind(':');
    if (pos == std::string::npos || pos + 1 >= s.size()) {
        return std::nullopt;
    }
    Endpoint ep;
    ep.host = s.substr(0, pos);
    ep.port = static_cast<std::uint16_t>(std::strtoul(s.c_str() + pos + 1, nullptr, 10));
    return ep;
}

} // namespace

SignalingClient::SignalingClient(UdpSocket& socket, Endpoint server, Bytes public_key,
                                 Signer signer)
    : socket_(socket), server_(std::move(server)), public_key_(std::move(public_key)),
      signer_(std::move(signer)) {
}

bool SignalingClient::send(const Packet& packet) {
    return socket_.send_to(server_, protocol::serialize(packet));
}

std::optional<Packet> SignalingClient::await_packet(PacketType expected, int timeout_ms) {
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::milliseconds(timeout_ms);
    while (true) {
        const auto now = clock::now();
        if (now >= deadline) {
            return std::nullopt;
        }
        const auto remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
        auto dg = socket_.receive(static_cast<int>(remaining));
        if (!dg) {
            return std::nullopt; // timeout
        }
        Packet pkt;
        if (protocol::deserialize(dg->data, pkt) && pkt.type == expected) {
            return pkt;
        }
        // Unrelated datagram: discard and keep waiting.
    }
}

bool SignalingClient::register_self(int timeout_ms, int attempts) {
    for (int attempt = 0; attempt < attempts; ++attempt) {
        // Step 1: request a challenge.
        Packet request;
        request.type = PacketType::Register;
        request.source = public_key_;
        if (!send(request)) {
            continue;
        }
        auto challenge = await_packet(PacketType::Hello, timeout_ms);
        if (!challenge) {
            continue;
        }

        // Step 2: return the signed challenge.
        Packet signed_register;
        signed_register.type = PacketType::Register;
        signed_register.source = public_key_;
        signed_register.payload = signer_ ? signer_(challenge->payload) : Bytes{};
        if (!send(signed_register)) {
            continue;
        }
        if (auto ack = await_packet(PacketType::Ack, timeout_ms)) {
            // STUN: the ACK payload is our public endpoint as the server saw it.
            public_endpoint_ = parse_endpoint(ack->payload);
            return true;
        }
        // No ACK: retry the whole handshake (the server issues a fresh challenge).
    }
    return false;
}

SignalingClient::DiscoverResult SignalingClient::discover(const Bytes& target_public_key,
                                                          int timeout_ms, int attempts) {
    for (int attempt = 0; attempt < attempts; ++attempt) {
        Packet query;
        query.type = PacketType::Discover;
        query.destination = target_public_key;
        if (!send(query)) {
            continue;
        }
        if (auto resp = await_packet(PacketType::Connect, timeout_ms)) {
            return DiscoverResult{true, parse_endpoint(resp->payload)};
        }
    }
    return DiscoverResult{false, std::nullopt};
}

bool SignalingClient::send_connect(const Bytes& target_public_key, const Bytes& info) {
    Packet connect;
    connect.type = PacketType::Connect;
    connect.source = public_key_;
    connect.destination = target_public_key;
    connect.payload = info;
    return send(connect);
}

bool SignalingClient::heartbeat(int timeout_ms) {
    Packet beat;
    beat.type = PacketType::Heartbeat;
    beat.source = public_key_;
    if (!send(beat)) {
        return false;
    }
    return await_packet(PacketType::Heartbeat, timeout_ms).has_value();
}

void SignalingClient::disconnect() {
    Packet bye;
    bye.type = PacketType::Disconnect;
    bye.source = public_key_;
    send(bye);
}

} // namespace openmesh::net
