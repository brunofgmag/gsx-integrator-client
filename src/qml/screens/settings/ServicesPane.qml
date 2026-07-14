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
        title: qsTr("Call GPU")
        caption: qsTr("Request ground power at the gate, disconnect before pushback")
        checked: root.settingsVm.callGpu
        onToggled: checked => root.settingsVm.callGpu = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Call catering")
        checked: root.settingsVm.callCatering
        onToggled: checked => root.settingsVm.callCatering = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Call lavatory service")
        caption: qsTr("After deboarding")
        checked: root.settingsVm.callLavatory
        onToggled: checked => root.settingsVm.callLavatory = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Call water service")
        caption: qsTr("After deboarding")
        checked: root.settingsVm.callWater
        onToggled: checked => root.settingsVm.callWater = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Call cleaning service")
        caption: qsTr("After deboarding")
        checked: root.settingsVm.callCleaning
        onToggled: checked => root.settingsVm.callCleaning = checked
    }

    Item {
        Layout.fillHeight: true
    }
}
