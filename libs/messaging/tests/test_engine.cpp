#include "openmesh/messaging/engine.hpp"

#include <cassert>

int main() {
    using namespace openmesh::messaging;

    Engine engine;
    bool delivered = false;
    engine.on_message([&](const Message& m) {
        delivered = m.outgoing && m.text == "hi";
    });

    engine.send_text({0x01}, "hi");
    assert(delivered);
    return 0;
}
