import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root

    property string text: ""

    implicitWidth: 15
    implicitHeight: 15
    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Help")
    Accessible.description: root.text

    readonly property bool shown: hover.hovered || root.activeFocus

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusSmall
        color: "transparent"
        border.color: root.shown ? Theme.accent : Theme.line
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "?"
            color: root.shown ? Theme.accent : Theme.muted
            font.pixelSize: 9
            font.bold: true
        }
    }

    HoverHandler {
        id: hover
    }

    ToolTip {
        visible: root.shown && root.text.length > 0
        delay: 150

        contentItem: Text {
            text: root.text
            color: Theme.tooltipFg
            font.pixelSize: 11
            lineHeight: 1.35
            wrapMode: Text.WordWrap
            width: Math.min(implicitWidth, 240)
        }

        background: Rectangle {
            color: Theme.tooltipBg
            border.color: Theme.tooltipBorder
            border.width: 1
            radius: Theme.radius
        }
    }
}
