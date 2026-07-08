import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root
    title: qsTr("Chat")

    // The peer we are chatting with (public-key fingerprint).
    property string peer: ""
    // Injected AppController bridge.
    property var controller: null

    signal back()

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "‹"   // ‹
                onClicked: root.back()
            }
            Label {
                text: root.peer
                elide: Label.ElideMiddle
                Layout.fillWidth: true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        ListView {
            id: messages
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: ListModel {}
            delegate: Label { text: model.text }
        }

        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: input
                Layout.fillWidth: true
                placeholderText: qsTr("Message")
            }
            Button {
                text: qsTr("Send")
                enabled: input.text.length > 0 && root.controller !== null
                onClicked: {
                    messages.model.append({ text: input.text })
                    root.controller.sendText(root.peer, input.text)
                    input.clear()
                }
            }
        }
    }
}
