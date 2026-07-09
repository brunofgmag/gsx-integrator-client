pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var settingsVm
    required property bool compact

    spacing: 8

    SettingRow {
        Layout.fillWidth: true
        title: qsTr("SimBrief Pilot ID")
        caption: qsTr("Used to fetch your latest OFP")
        helpText: ""

        TextField {
            id: pilotIdField
            width: 110
            height: 32
            text: root.settingsVm.simbriefPilotIdText
            color: Theme.text
            font.pixelSize: 12
            horizontalAlignment: TextInput.AlignRight
            leftPadding: 10
            validator: IntValidator {
                bottom: 1
            }
            inputMethodHints: Qt.ImhDigitsOnly
            onTextEdited: root.settingsVm.simbriefPilotIdText = text

            background: Rectangle {
                radius: Theme.radiusSmall
                color: Theme.panel2
                border.color: pilotIdField.activeFocus ? Theme.accent : Theme.line
                border.width: 1
            }
        }
    }

    SettingRow {
        Layout.fillWidth: true
        title: qsTr("Fuel rate")
        caption: qsTr("Refueling speed")
        helpText: ""

        TextField {
            id: fuelRateField
            width: 70
            height: 32
            text: root.settingsVm.fuelRateText
            color: Theme.text
            font.pixelSize: 12
            horizontalAlignment: TextInput.AlignRight
            rightPadding: 10
            validator: DoubleValidator {
                bottom: 0.1
                top: 1000
                notation: DoubleValidator.StandardNotation
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
            text: qsTr("kg/s")
            color: Theme.muted
            font.pixelSize: 10
            font.letterSpacing: 0.8
            font.capitalization: Font.AllUppercase
        }
    }

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Auto-select GSX choice")
        caption: qsTr("Always pick the \"GSX choice\" menu option")
        helpText: qsTr("When on, the integrator automatically selects any GSX menu option marked \"GSX choice\", even on menus you open manually.")
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

    SettingRow {
        Layout.fillWidth: true
        title: qsTr("Updates")
        caption: qsTr("How new versions are installed")
        helpText: qsTr("Auto downloads updates and applies them when the app closes. Notify only shows an alert in the header. Manual never checks on its own.")

        SegmentedControl {
            anchors.verticalCenter: parent.verticalCenter
            model: [qsTr("Auto"), qsTr("Notify"), qsTr("Manual")]
            currentIndex: root.settingsVm.updateMode
            onActivated: index => root.settingsVm.updateMode = index
        }
    }

    SettingRow {
        Layout.fillWidth: true
        title: qsTr("Theme")
        caption: qsTr("\"Windows\" follows the system")

        SegmentedControl {
            anchors.verticalCenter: parent.verticalCenter
            model: [qsTr("Light"), qsTr("Dark"), qsTr("Windows")]
            currentIndex: root.settingsVm.themeMode
            onActivated: index => root.settingsVm.themeMode = index
        }
    }

    SettingRow {
        Layout.fillWidth: true
        title: qsTr("Language")
        caption: qsTr("\"Windows\" uses the system language")

        SegmentedControl {
            anchors.verticalCenter: parent.verticalCenter
            model: [qsTr("Windows"), "English", "Português (BR)"]
            currentIndex: root.settingsVm.language === "system" ? 0
                        : root.settingsVm.language === "pt_BR" ? 2 : 1
            onActivated: index => root.settingsVm.language = ["system", "en", "pt_BR"][index]
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.topMargin: 8
        implicitHeight: saveButton.implicitHeight

        readonly property bool showValidation: !root.settingsVm.canSave
                                               && root.settingsVm.validationMessage.length > 0

        Text {
            id: saveFeedback
            anchors.left: parent.left
            anchors.right: saveButton.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: parent.showValidation
                  ? root.settingsVm.validationMessage
                  : root.settingsVm.saveMessage
            color: (parent.showValidation || root.settingsVm.saveError) ? Theme.red : Theme.muted
            font.pixelSize: 11
            font.capitalization: Font.AllUppercase
            elide: Text.ElideRight
            opacity: (parent.showValidation || text.length > 0) ? 1 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: 200
                }
            }

            Timer {
                id: feedbackTimer
                interval: 2500
                running: !saveFeedback.parent.showValidation && saveFeedback.text.length > 0
                onTriggered: root.settingsVm.clearSaveMessage()
            }
        }

        ActionButton {
            id: saveButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Save settings")
            enabled: root.settingsVm.canSave

            onClicked: root.settingsVm.save()
        }
    }
}
