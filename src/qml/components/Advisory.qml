import QtQuick

Item {
    id: root

    property string text: ""
    property bool hideable: false
    property bool hidden: false

    visible: root.text.length > 0
    implicitHeight: root.hidden ? collapsed.implicitHeight : strip.implicitHeight

    Rectangle {
        id: strip
        visible: !root.hidden
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
                       - (hideButton.visible ? hideButton.width + 10 : 0)
                text: root.text
                color: Theme.tipFg
                font.pixelSize: 11
                lineHeight: 1.3
                wrapMode: Text.WordWrap
            }

            ActionButton {
                id: hideButton
                anchors.verticalCenter: parent.verticalCenter
                visible: root.hideable
                secondary: true
                small: true
                text: qsTr("Hide")
                onClicked: root.hidden = true
            }
        }
    }

    Rectangle {
        id: collapsed
        visible: root.hidden
        width: parent.width
        implicitHeight: 40
        radius: Theme.radius
        color: "transparent"
        border.color: Theme.line
        border.width: 1

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Advisories hidden")
            color: Theme.muted
            font.pixelSize: 10
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
        }

        ActionButton {
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            secondary: true
            small: true
            text: qsTr("Show")
            onClicked: root.hidden = false
        }
    }
}
