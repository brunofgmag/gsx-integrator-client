pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var settingsVm

    readonly property bool ghost: settingsVm.profileUseGlobal

    spacing: 8

    onVisibleChanged: {
        if (visible) {
            root.settingsVm.selectDetectedProfile();
        }
    }

    Flow {
        Layout.fillWidth: true
        spacing: 4

        Repeater {
            model: root.settingsVm.profileModel

            Rectangle {
                id: chip

                required property int index
                required property var modelData
                readonly property bool selected: root.settingsVm.selectedProfileIndex === index
                readonly property bool detected: root.settingsVm.detectedProfileIndex === index

                width: chipText.implicitWidth + 20
                height: 24
                radius: Theme.radiusSmall
                color: selected ? Theme.accent : "transparent"
                border.color: selected ? Theme.accent : Theme.line
                border.width: 1

                Text {
                    id: chipText
                    anchors.centerIn: parent
                    text: (chip.detected ? "● " : "") + chip.modelData.shortCode
                    color: chip.selected ? Theme.accentText : Theme.muted
                    font.pixelSize: 10
                    font.letterSpacing: 1.0
                }

                TapHandler {
                    onTapped: root.settingsVm.selectedProfileIndex = chip.index
                }

                ToolTip.visible: chipHover.hovered
                ToolTip.delay: 400
                ToolTip.text: chip.modelData.name

                HoverHandler {
                    id: chipHover
                }
            }
        }
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Use global settings")
        caption: qsTr("Mirror the global configuration for this aircraft")
        checked: root.settingsVm.profileUseGlobal
        onToggled: checked => root.settingsVm.profileUseGlobal = checked
    }

    SettingRow {
        Layout.fillWidth: true
        enabled: !root.ghost && root.settingsVm.profileFuelEditable
        title: qsTr("Fuel rate")
        caption: qsTr("Refueling speed")
        helpText: root.settingsVm.profileFuelEditable
                  ? qsTr("Refueling speed used when this aircraft is loaded by the client.")
                  : qsTr("This aircraft's refueling pace is driven by GSX or by the aircraft itself, so the rate is not configurable.")

        TextField {
            id: profileFuelField
            visible: root.settingsVm.profileFuelEditable
            width: 70
            height: 32
            text: root.ghost ? root.settingsVm.fuelRateText
                             : root.settingsVm.profileFuelRateText
            color: Theme.text
            font.pixelSize: 12
            horizontalAlignment: TextInput.AlignRight
            rightPadding: 10
            opacity: enabled ? 1.0 : 0.45
            validator: DoubleValidator {
                bottom: 0.1
                top: 1000
                notation: DoubleValidator.StandardNotation
            }
            onTextEdited: root.settingsVm.profileFuelRateText = text

            background: Rectangle {
                radius: Theme.radiusSmall
                color: Theme.panel2
                border.color: profileFuelField.activeFocus ? Theme.accent : Theme.line
                border.width: 1
            }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.settingsVm.profileFuelEditable
                  ? qsTr("kg/s")
                  : root.settingsVm.profileFuelBadge
            color: Theme.muted
            font.pixelSize: 10
            font.letterSpacing: 0.8
            font.capitalization: Font.AllUppercase
        }
    }

    SwitchRow {
        Layout.fillWidth: true
        enabled: !root.ghost
        title: qsTr("Skip aircraft repositioning")
        checked: root.ghost ? root.settingsVm.skipReposition
                            : root.settingsVm.profileSkipReposition
        onToggled: checked => root.settingsVm.profileSkipReposition = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        enabled: !root.ghost
        title: qsTr("Call GPU")
        checked: root.ghost ? root.settingsVm.callGpu
                            : root.settingsVm.profileCallGpu
        onToggled: checked => root.settingsVm.profileCallGpu = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        enabled: !root.ghost
        title: qsTr("Call catering")
        helpText: qsTr("Cargo aircraft skip catering automatically, even when this is on.")
        checked: root.ghost ? root.settingsVm.callCatering
                            : root.settingsVm.profileCallCatering
        onToggled: checked => root.settingsVm.profileCallCatering = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        enabled: !root.ghost
        title: qsTr("Call lavatory service")
        checked: root.ghost ? root.settingsVm.callLavatory
                            : root.settingsVm.profileCallLavatory
        onToggled: checked => root.settingsVm.profileCallLavatory = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        enabled: !root.ghost
        title: qsTr("Call water service")
        checked: root.ghost ? root.settingsVm.callWater
                            : root.settingsVm.profileCallWater
        onToggled: checked => root.settingsVm.profileCallWater = checked
    }

    SwitchRow {
        Layout.fillWidth: true
        enabled: !root.ghost
        title: qsTr("Call cleaning service")
        checked: root.ghost ? root.settingsVm.callCleaning
                            : root.settingsVm.profileCallCleaning
        onToggled: checked => root.settingsVm.profileCallCleaning = checked
    }

    Row {
        Layout.fillWidth: true
        spacing: 6

        ConfirmButton {
            baseText: qsTr("Set as global default")
            enabled: !root.ghost
            onConfirmed: root.settingsVm.setProfileAsGlobalDefault()
        }

        ConfirmButton {
            baseText: qsTr("Apply to all profiles")
            onConfirmed: root.settingsVm.applyProfileToAllProfiles()
        }
    }

    Item {
        Layout.fillHeight: true
    }
}
