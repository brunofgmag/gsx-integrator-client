import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root

    property bool active: false
    property string tip: ""

    implicitWidth: 34
    implicitHeight: 30

    contentItem: Text {
        renderType: Text.QtRendering
        text: root.text
        color: root.active ? Theme.accentText : (root.enabled ? Theme.muted : Theme.faint)
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: root.active ? Theme.accent : (root.hovered ? Theme.panel2 : "transparent")
        border.color: root.active ? Theme.accent : Theme.line
        border.width: 1
    }

    ToolTip.visible: hovered && tip.length > 0
    ToolTip.text: tip
    ToolTip.delay: 400
}
