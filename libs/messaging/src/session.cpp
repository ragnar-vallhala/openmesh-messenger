#include "openmesh/messaging/session.hpp"

#include "openmesh/crypto/aead.hpp"
#include "openmesh/crypto/key_agreement.hpp"

#include <utility>

namespace openmesh::messaging {

Session::Session(Bytes local_public, Bytes local_secret, Bytes remote_public)
    : local_public_(std::move(local_public)), remote_public_(std::move(remote_public)) {
    if (auto key = crypto::agree_shared_key(local_secret, remote_public_)) {
        shared_key_ = std::move(*key);
        valid_ = true;
    }
}

protocol::Packet Session::encrypt(const Bytes& plaintext) {
    return encrypt_as(protocol::PacketType::Message, plaintext);
}

protocol::Packet Session::encrypt_as(protocol::PacketType type, const Bytes& plaintext) {
    protocol::Packet packet;
    packet.type = type;
    packet.flags = protocol::kFlagEncrypted;
    packet.counter = send_counter_++;
    packet.source = local_public_;
    packet.destination = remote_public_;
    if (!valid_) {
        return packet;
    }
    // The envelope (header + routing) authenticates the ciphertext as AAD. It is
    // computed before the payload exists — associated_data() ignores the payload.
    const Bytes aad = protocol::associated_data(packet);
    packet.payload = crypto::seal(shared_key_, plaintext, aad);
    return packet;
}

std::optional<Bytes> Session::decrypt(const protocol::Packet& packet) {
    if (!valid_) {
        return std::nullopt;
    }
    if (!packet.encrypted()) {
        return std::nullopt;
    }
    // Must be from our peer and addressed to us.
    if (packet.source != remote_public_ || packet.destination != local_public_) {
        return std::nullopt;
    }
    // Replay/ordering: the counter must strictly increase (wire-format §7).
    if (have_received_ && packet.counter <= last_received_) {
        return std::nullopt;
    }

    const Bytes aad = protocol::associated_data(packet);
    auto plaintext = crypto::open(shared_key_, packet.payload, aad);
    if (!plaintext) {
        return std::nullopt; // authentication failed (tampered envelope or ciphertext)
    }

    have_received_ = true;
    last_received_ = packet.counter;
    return plaintext;
}

} // namespace openmesh::messaging
