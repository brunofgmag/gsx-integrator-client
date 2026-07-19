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
        title: qsTr("Call GPU & chocks")
        caption: qsTr("Place ground power and chocks at the gate, remove before pushback")
        helpText: qsTr("Chocks are placed only on aircraft that support external chocks control.")
        checked: root.settingsVm.callGpu
        onToggled: checked => root.settingsVm.callGpu = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Call GPU & chocks on arrival")
        caption: qsTr("Place ground power and chocks after landing, once engines are off and the parking brake is set")
        helpText: qsTr("Chocks are placed only on aircraft that support external chocks control.")
        checked: root.settingsVm.callGpuOnArrival
        onToggled: checked => root.settingsVm.callGpuOnArrival = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Call catering")
        helpText: qsTr("Cargo aircraft skip catering automatically, even when this is on.")
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

    SettingRow {
        Layout.fillWidth: true
        title: qsTr("Crew boarding")
        caption: qsTr("Answer when GSX asks who boards the aircraft")
        helpText: qsTr("The GSX option to ignore Crew/Pilots boarding must be disabled, otherwise this prompt never appears.")

        SegmentedControl {
            anchors.verticalCenter: parent.verticalCenter
            model: [qsTr("Nobody"), qsTr("Crew"), qsTr("Pilots"), qsTr("Both")]
            currentIndex: root.settingsVm.crewBoarding
            onActivated: index => root.settingsVm.crewBoarding = index
        }
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Accept de-ice requests")
        caption: qsTr("Answer \"Yes\" when GSX offers de-icing treatment")
        helpText: qsTr("Overrides the GSX choice for the ice-warning popup, which otherwise declines de-icing.")
        checked: root.settingsVm.autoDeice
        onToggled: checked => root.settingsVm.autoDeice = checked
    }

    Item {
        Layout.fillHeight: true
    }
}
