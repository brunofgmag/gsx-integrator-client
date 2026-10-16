pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var settingsVm
    property bool trayAvailable: true

    spacing: 8

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Close to tray")
        caption: root.trayAvailable
                 ? qsTr("Closing hides the window to the tray")
                 : qsTr("No system tray found, so closing asks to quit")
        checked: root.settingsVm.closeToTray
        onToggled: checked => root.settingsVm.closeToTray = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Minimize to tray")
        caption: root.trayAvailable
                 ? qsTr("Minimizing hides the window to the tray")
                 : qsTr("No system tray found, so minimizing keeps the window")
        checked: root.settingsVm.minimizeToTray
        onToggled: checked => root.settingsVm.minimizeToTray = checked
    }

    Item {
        Layout.fillHeight: true
    }
}
