import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root

    property bool secondary: false
    property bool small: false

    implicitHeight: small ? 30 : 36
    leftPadding: small ? 14 : 18
    rightPadding: small ? 14 : 18

    contentItem: Text {
        text: root.text
        color: root.secondary ? Theme.muted : Theme.accent
        font.pixelSize: root.small ? 10 : 11
        font.bold: !root.secondary
        font.letterSpacing: 1.2
        font.capitalization: Font.AllUppercase
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: root.enabled ? 1.0 : 0.5
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: root.enabled && root.down ? Qt.alpha(Theme.accent, 0.22)
             : root.enabled && root.hovered ? Qt.alpha(Theme.accent, 0.12)
             : "transparent"
        border.color: root.secondary ? Theme.line : Theme.accent
        border.width: 1
        opacity: root.enabled ? 1.0 : 0.5
    }
}
