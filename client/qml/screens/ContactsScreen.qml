import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root
    title: qsTr("Contacts")

    // Emitted when the user opens a conversation with a contact.
    signal openChat(string fingerprint)

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                text: root.title
                font.pixelSize: 20
                Layout.leftMargin: 12
                Layout.fillWidth: true
            }
        }
    }

    // Placeholder contact list. Replace with a real model backed by
    // openmesh::storage via a C++ item model in client/src/models/.
    ListView {
        anchors.fill: parent
        model: ListModel {
            ListElement { nickname: "alice"; fingerprint: "om:alice-fingerprint" }
            ListElement { nickname: "bob";   fingerprint: "om:bob-fingerprint" }
        }
        delegate: ItemDelegate {
            width: ListView.view.width
            text: model.nickname + "  —  " + model.fingerprint
            onClicked: root.openChat(model.fingerprint)
        }
    }
}
