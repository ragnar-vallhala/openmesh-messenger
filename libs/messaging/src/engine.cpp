#include "openmesh/messaging/engine.hpp"

namespace openmesh::messaging {

void Engine::send_text(const std::vector<std::uint8_t>& peer, const std::string& text) {
    // Skeleton: echo back through the handler so higher layers can be wired up
    // and tested. The real path is:
    //   1. look up / establish an encrypted session (crypto)
    //   2. serialize + encrypt a MESSAGE packet (protocol + crypto)
    //   3. transmit via the networking engine (net)
    //   4. persist locally and update delivery status (storage)
    if (on_message_) {
        Message m;
        m.peer = peer;
        m.text = text;
        m.outgoing = true;
        on_message_(m);
    }
}

} // namespace openmesh::messaging
