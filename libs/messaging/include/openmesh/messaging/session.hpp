#pragma once

#include "openmesh/protocol/packet.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace openmesh::messaging {

using Bytes = std::vector<std::uint8_t>;

// An end-to-end encrypted session with one peer (SRS §5, FR-4, FR-5).
//
// Combines the crypto primitives with the wire format: it derives a shared key
// from the two identities (X25519 key agreement), then encrypts/decrypts MESSAGE
// packets with XChaCha20-Poly1305, binding the packet envelope as associated
// data so tampering is rejected. A per-session monotonic counter provides message
// ordering and replay protection (wire-format §7): a received counter must be
// strictly greater than the last accepted one.
//
// NOTE: the key agreement is static ECDH; this is not yet forward-secret. The
// Double Ratchet (docs/crypto/design.md §6) will layer on top later.
class Session {
public:
    // `local_public`/`local_secret` are our Ed25519 identity; `remote_public` is
    // the peer's Ed25519 public key. Check valid() before use.
    Session(Bytes local_public, Bytes local_secret, Bytes remote_public);

    [[nodiscard]] bool valid() const { return valid_; }
    [[nodiscard]] const Bytes& remote_public_key() const { return remote_public_; }

    // Encrypt `plaintext` into a MESSAGE packet addressed to the peer, advancing
    // the send counter. Returns a packet with an empty payload if !valid().
    protocol::Packet encrypt(const Bytes& plaintext);

    // Decrypt a received MESSAGE packet. Returns nullopt if the packet is not a
    // valid, in-order, authentic message for this session (wrong type/flags,
    // wrong peer, replayed/old counter, or failed authentication).
    std::optional<Bytes> decrypt(const protocol::Packet& packet);

private:
    Bytes local_public_;
    Bytes remote_public_;
    Bytes shared_key_;
    bool valid_ = false;

    std::uint64_t send_counter_ = 0;
    bool have_received_ = false;
    std::uint64_t last_received_ = 0;
};

} // namespace openmesh::messaging
