#include "openmesh/net/signaling_client.hpp"

#include <cassert>

using namespace openmesh::net;

// The client's failure paths don't need a server or crypto: pointed at an
// address where nothing answers, requests must time out cleanly. (The happy-path
// handshake against a real SignalingServer is covered in the signaling e2e test.)
int main() {
    bool signer_called = false;
    SignalingClient::Signer signer = [&](const Bytes&) {
        signer_called = true;
        return Bytes(64, 0);
    };

    // Port 9 (discard) on loopback: no signaling server listening.
    SignalingClient client(Endpoint{"127.0.0.1", 9}, Bytes(32, 0x11), signer);

    // REGISTER: no challenge ever arrives -> false, fast.
    assert(!client.register_self(/*timeout_ms=*/100, /*attempts=*/1));
    assert(!signer_called); // never reached the signing step

    // DISCOVER: no response -> responded == false.
    auto result = client.discover(Bytes(32, 0x22), /*timeout_ms=*/100, /*attempts=*/1);
    assert(!result.responded);
    assert(!result.endpoint.has_value());

    // Fire-and-forget calls just need to send without throwing.
    assert(client.send_connect(Bytes(32, 0x22), Bytes{'x'}));
    client.disconnect();

    // The socket opened lazily on first send; it now has a local endpoint.
    assert(client.local_endpoint().has_value());
    return 0;
}
