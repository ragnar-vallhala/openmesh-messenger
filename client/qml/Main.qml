import QtQuick
import QtQuick.Controls
import OpenMesh

ApplicationWindow {
    id: window
    width: 420
    height: 720
    visible: true
    title: qsTr("OpenMesh Messenger")

    // The single bridge to the Messaging Engine.
    AppController {
        id: app
        onMessageReceived: (peer, text, outgoing) => console.log(
            (outgoing ? "-> " : "<- ") + peer + ": " + text)
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: contactsScreen
    }

    Component {
        id: contactsScreen
        ContactsScreen {
            onOpenChat: (fingerprint) => stack.push(chatScreen, { peer: fingerprint })
        }
    }

    Component {
        id: chatScreen
        ChatScreen {
            controller: app
            onBack: stack.pop()
        }
    }
}
