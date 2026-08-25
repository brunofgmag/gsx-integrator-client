import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root

    property bool secondary: false
    property bool small: false
    property color tint: Theme.accent

    implicitHeight: small ? 30 : 36
    leftPadding: small ? 14 : 18
    rightPadding: small ? 14 : 18

    contentItem: Text {
        text: root.text
        color: !root.enabled ? Theme.faint
             : root.secondary ? Theme.muted
             : root.tint
        font.pixelSize: root.small ? 10 : 11
        font.bold: !root.secondary
        font.letterSpacing: 1.2
        font.capitalization: Font.AllUppercase
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: !root.enabled ? Theme.panel2
             : root.down ? Qt.alpha(root.tint, 0.22)
             : root.hovered ? Qt.alpha(root.tint, 0.12)
             : "transparent"
        border.color: !root.enabled ? Theme.panel2
                    : root.secondary ? Theme.line
                    : root.tint
        border.width: 1
    }
}
