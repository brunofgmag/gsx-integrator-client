pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var settingsVm

    spacing: 8

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Close to tray")
        caption: qsTr("Closing hides the window to the tray")
        checked: root.settingsVm.closeToTray
        onToggled: checked => root.settingsVm.closeToTray = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Minimize to tray")
        caption: qsTr("Minimizing hides the window to the tray")
        checked: root.settingsVm.minimizeToTray
        onToggled: checked => root.settingsVm.minimizeToTray = checked
    }

    Item {
        Layout.fillHeight: true
    }
}
