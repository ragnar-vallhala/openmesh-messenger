#include "bridge/app_controller.hpp"

#include "openmesh/core/hex.hpp"

AppController::AppController(QObject* parent)
    : QObject(parent), identity_(openmesh::crypto::generate_identity()),
      engine_(identity_.public_key, identity_.secret_key) {
    engine_.on_message([this](const openmesh::messaging::Message& m) {
        emit messageReceived(QString::fromStdString(openmesh::core::to_hex(m.peer)),
                             QString::fromUtf8(reinterpret_cast<const char*>(m.plaintext.data()),
                                               static_cast<int>(m.plaintext.size())),
                             m.outgoing);
    });
}

QString AppController::fingerprint() const {
    return QString::fromStdString(identity_.fingerprint());
}

void AppController::sendText(const QString& peerFingerprint, const QString& text) {
    // Placeholder: until the UI is wired to peer discovery + sessions, echo the
    // outgoing message straight back so the chat screen is usable.
    emit messageReceived(peerFingerprint, text, /*outgoing=*/true);
}
