import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root

    property string tip: ""

    implicitWidth: label.implicitWidth + 22
    implicitHeight: 30

    contentItem: Text {
        id: label
        renderType: Text.QtRendering
        text: root.text
        color: root.down || root.hovered ? Theme.accentText : Theme.accent
        font.pixelSize: 10
        font.letterSpacing: 1
        font.capitalization: Font.AllUppercase
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: root.down || root.hovered ? Theme.accent : "transparent"
        border.color: Theme.accent
        border.width: 1
    }

    ToolTip.visible: hovered && tip.length > 0
    ToolTip.text: tip
    ToolTip.delay: 400
}
