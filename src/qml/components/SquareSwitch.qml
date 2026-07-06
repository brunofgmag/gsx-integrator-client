import QtQuick
import QtQuick.Controls.Basic

Switch {
    id: root

    implicitWidth: 38
    implicitHeight: 20

    indicator: Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.panel2
        border.color: root.checked ? Theme.accent : Theme.line
        border.width: 1
        opacity: root.enabled ? 1.0 : 0.45

        Rectangle {
            x: root.checked ? parent.width - width - 4 : 4
            anchors.verticalCenter: parent.verticalCenter
            width: 12
            height: 12
            radius: 2
            color: root.checked ? Theme.accent : Theme.muted

            Behavior on x {
                NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
            }
        }
    }
}
