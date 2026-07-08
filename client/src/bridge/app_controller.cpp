#include "bridge/app_controller.hpp"

#include <string>
#include <vector>

AppController::AppController(QObject* parent) : QObject(parent) {
    engine_.on_message([this](const openmesh::messaging::Message& m) {
        emit messageReceived(QString::fromUtf8(reinterpret_cast<const char*>(m.peer.data()),
                                               static_cast<int>(m.peer.size())),
                             QString::fromStdString(m.text), m.outgoing);
    });
}

void AppController::sendText(const QString& peerFingerprint, const QString& text) {
    // Placeholder: treat the fingerprint string bytes as the peer key for now.
    const std::string fp = peerFingerprint.toStdString();
    std::vector<std::uint8_t> peer(fp.begin(), fp.end());
    engine_.send_text(peer, text.toStdString());
}
