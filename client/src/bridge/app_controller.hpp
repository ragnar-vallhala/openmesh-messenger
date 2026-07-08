#pragma once

#include "openmesh/messaging/engine.hpp"

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

// The single QML-facing controller (bridge layer). This is the only place where
// Qt types meet the UI-independent Messaging Engine. QML calls into it; it
// forwards to openmesh::messaging::Engine and emits signals back to QML.
class AppController : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit AppController(QObject* parent = nullptr);

    // Placeholder action wired to the QML UI.
    Q_INVOKABLE void sendText(const QString& peerFingerprint, const QString& text);

signals:
    void messageReceived(const QString& peerFingerprint, const QString& text, bool outgoing);

private:
    openmesh::messaging::Engine engine_;
};
