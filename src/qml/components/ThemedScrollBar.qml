import QtQuick
import QtQuick.Controls.Basic

ScrollBar {
    id: root

    property bool needed: false

    policy: root.needed ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
    width: 10
    padding: 1
    minimumSize: Math.min(1, 32 / Math.max(1, root.height))

    background: Rectangle {
        radius: Theme.radiusSmall
        color: Theme.panel2
        border.color: Theme.line
        border.width: 1
    }

    contentItem: Rectangle {
        implicitWidth: 8
        radius: Theme.radiusSmall
        color: root.hovered || root.pressed ? Theme.accent : Theme.muted
    }
}
