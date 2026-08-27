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
        caption: qsTr("Places ground power and chocks at the gate, removes them before pushback")
        helpText: qsTr("Only on aircraft that accept external chocks control.")
        checked: root.settingsVm.callGpu
        onToggled: checked => root.settingsVm.callGpu = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Call GPU & chocks on arrival")
        caption: qsTr("Places ground power and chocks with engines off and the brake set")
        helpText: qsTr("Only on aircraft that accept external chocks control.")
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
        caption: qsTr("Answers when GSX asks who boards")
        helpText: qsTr("The GSX option to ignore Crew/Pilots boarding must be disabled, otherwise this prompt never appears.")

        SelectBox {
            anchors.verticalCenter: parent.verticalCenter
            model: [qsTr("Nobody"), qsTr("Crew"), qsTr("Pilots"), qsTr("Both")]
            currentIndex: root.settingsVm.crewBoarding
            onActivated: index => root.settingsVm.crewBoarding = index
        }
    }

    SettingRow {
        Layout.fillWidth: true
        title: qsTr("Crew deboarding")
        caption: qsTr("Answers when GSX asks who deboards")
        helpText: qsTr("The GSX option to ignore Crew/Pilots boarding must be disabled, otherwise this prompt never appears.")

        SelectBox {
            anchors.verticalCenter: parent.verticalCenter
            model: [qsTr("Nobody"), qsTr("Crew"), qsTr("Pilots"), qsTr("Both")]
            currentIndex: root.settingsVm.crewDeboarding
            onActivated: index => root.settingsVm.crewDeboarding = index
        }
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Always use aircraft stairs")
        caption: qsTr("Answers \"Yes\" when GSX offers the aircraft's own airstairs")
        helpText: qsTr("GSX only asks this on aircraft that have their own airstairs. When off, the integrator answers with the airport stairs.")
        checked: root.settingsVm.useAircraftStairs
        onToggled: checked => root.settingsVm.useAircraftStairs = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Accept de-ice requests")
        caption: qsTr("Answers \"Yes\" when GSX offers de-icing")
        helpText: qsTr("Overrides the GSX choice for the ice-warning popup, which otherwise declines de-icing.")
        checked: root.settingsVm.autoDeice
        onToggled: checked => root.settingsVm.autoDeice = checked
    }

    Item {
        Layout.fillHeight: true
    }
}
