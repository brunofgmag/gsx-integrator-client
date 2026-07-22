pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var settingsVm

    spacing: 8

    SettingRow {
        Layout.fillWidth: true
        title: qsTr("Fuel rate")
        caption: qsTr("Refueling speed")
        helpText: qsTr("Used only by aircraft that don't support refueling through GSX. When GSX drives the fuel truck, the client follows the GSX pace and the rate shows as Auto.")

        TextField {
            id: fuelRateField
            width: 70
            height: 32
            text: root.settingsVm.fuelRateText
            color: Theme.text
            font.pixelSize: 12
            horizontalAlignment: TextInput.AlignRight
            rightPadding: 10
            validator: IntValidator {
                bottom: 1
                top: root.settingsVm.weightIsLb ? Math.round(root.settingsVm.kgToLb(1000)) : 1000
            }
            onTextEdited: root.settingsVm.fuelRateText = text

            background: Rectangle {
                radius: Theme.radiusSmall
                color: Theme.panel2
                border.color: fuelRateField.activeFocus ? Theme.accent : Theme.line
                border.width: 1
            }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.settingsVm.fuelRateUnitText
            color: Theme.muted
            font.pixelSize: 10
            font.letterSpacing: 0.8
            font.capitalization: Font.AllUppercase
        }
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Accept actions automatically")
        caption: qsTr("Picks the recommended GSX menu option for you")
        helpText: qsTr("When on, the integrator answers GSX menus on its own: any option marked \"GSX choice\" and the Simbrief block fuel refueling level, even on menus you open manually.")
        checked: root.settingsVm.autoSelectGsxChoice
        onToggled: checked => root.settingsVm.autoSelectGsxChoice = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Auto-start turnaround")
        caption: qsTr("Start without pressing anything")
        helpText: qsTr("Starts the turnaround on its own once it detects a supported aircraft and a flight plan.")
        checked: root.settingsVm.autoStartFlow
        onToggled: checked => root.settingsVm.autoStartFlow = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Auto-start loading")
        caption: qsTr("Request refueling without pressing anything")
        helpText: qsTr("When off, the turnaround holds at \"Requesting fuel\" until you press Start Loading or activate the aircraft's SmartSwitch. Refueling requested manually in the GSX menu still resumes the flow.")
        checked: root.settingsVm.autoStartLoading
        onToggled: checked => root.settingsVm.autoStartLoading = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Skip aircraft repositioning")
        checked: root.settingsVm.skipReposition
        onToggled: checked => root.settingsVm.skipReposition = checked
    }

    Item {
        Layout.fillHeight: true
    }
}
