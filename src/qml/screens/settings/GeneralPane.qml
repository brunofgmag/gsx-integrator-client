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
            echoMode: root.settingsVm.streamerMode ? TextInput.Password : TextInput.Normal
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

    SwitchRow {
        Layout.fillWidth: true
        title: qsTr("Streamer mode")
        caption: qsTr("Hides personal IDs and credentials")
        checked: root.settingsVm.streamerMode
        onToggled: checked => root.settingsVm.streamerMode = checked
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
        Layout.fillHeight: true
    }
}
