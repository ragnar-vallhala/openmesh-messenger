#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace openmesh::messaging {

// A chat message as seen by the application layer (SRS FR-4).
struct Message {
    std::vector<std::uint8_t> peer;      // contact public key
    std::string   text;                  // plaintext (only ever in memory/local)
    std::int64_t  timestamp = 0;         // Unix epoch ms
    bool          outgoing  = false;
};

// The UI-independent Messaging Engine (SRS §10).
//
// Skeleton only: the interface is intentionally free of Qt/UI types. The UI
// subscribes via callbacks; the engine coordinates crypto/net/storage.
class Engine {
public:
    using MessageHandler = std::function<void(const Message&)>;

    // Register a callback fired when a message arrives from a peer.
    void on_message(MessageHandler handler) { on_message_ = std::move(handler); }

    // Send a text message to an accepted contact (SRS FR-4).
    // TODO: encrypt via crypto, transmit via net, persist via storage.
    void send_text(const std::vector<std::uint8_t>& peer, const std::string& text);

private:
    MessageHandler on_message_;
};

} // namespace openmesh::messaging
