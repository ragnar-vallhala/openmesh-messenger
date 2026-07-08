#pragma once

#include "openmesh/crypto/identity.hpp"
#include "openmesh/messaging/engine.hpp"

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

// The single QML-facing controller (bridge layer). This is the only place where
// Qt types meet the UI-independent Messaging Engine. QML calls into it; it
// forwards to openmesh::messaging::Engine and emits signals back to QML.
//
// NOTE: the engine holds a freshly generated identity, but the UI is not yet
// wired to real peer discovery/transport — sendText echoes locally for now.
// Connecting the UI to signaling + sessions is a follow-up.
class AppController : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit AppController(QObject* parent = nullptr);

    // This device's identity fingerprint, for display/sharing (SRS FR-1).
    Q_INVOKABLE QString fingerprint() const;

    // Placeholder action wired to the QML UI (local echo until networking lands).
    Q_INVOKABLE void sendText(const QString& peerFingerprint, const QString& text);

signals:
    void messageReceived(const QString& peerFingerprint, const QString& text, bool outgoing);

private:
    openmesh::crypto::Identity identity_;
    openmesh::messaging::Engine engine_;
};
