import QtQuick

Item {
    id: root

    property string text: ""
    property string actionText: ""

    signal actionTriggered()

    visible: root.text.length > 0
    implicitHeight: strip.implicitHeight

    Rectangle {
        id: strip
        width: parent.width
        implicitHeight: Math.max(stripRow.implicitHeight + 18, 38)
        radius: Theme.radius
        color: Theme.tipBg
        border.color: Theme.line
        border.width: 1

        Row {
            id: stripRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: badge.implicitWidth + 14
                height: badge.implicitHeight + 6
                radius: Theme.radiusSmall
                color: Theme.amber

                Text {
                    id: badge
                    anchors.centerIn: parent
                    text: qsTr("Advisory")
                    color: Theme.bg
                    font.pixelSize: 9
                    font.bold: true
                    font.letterSpacing: 1.2
                    font.capitalization: Font.AllUppercase
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - badge.implicitWidth - 24
                       - (actionButton.visible ? actionButton.width + 10 : 0)
                text: root.text
                color: Theme.tipFg
                font.pixelSize: 11
                lineHeight: 1.3
                wrapMode: Text.WordWrap
            }

            ActionButton {
                id: actionButton
                anchors.verticalCenter: parent.verticalCenter
                visible: root.actionText.length > 0
                small: true
                text: root.actionText
                onClicked: root.actionTriggered()
            }
        }
    }
}
