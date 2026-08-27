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
        caption: qsTr("Fetches your latest OFP")
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
        title: qsTr("GSX panel")
        caption: qsTr("When the client opens the GSX window in the sim")
        helpText: qsTr("On pushback opens the panel for the destination menu and closes it once the push begins. Never leaves the panel alone. All requests opens it whenever the client asks GSX for anything.")

        SelectBox {
            anchors.verticalCenter: parent.verticalCenter
            model: [qsTr("Never"), qsTr("On pushback"), qsTr("All requests")]
            currentIndex: root.settingsVm.gsxPanelMode
            onActivated: index => root.settingsVm.gsxPanelMode = index
        }
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
        title: qsTr("Weight unit")
        caption: qsTr("Units shown for fuel and payload")
        helpText: qsTr("In Auto the unit comes from SimBrief, when a Pilot ID is set, or from the aircraft's flight plan when the client can read it. With neither, the client uses KG.")

        SegmentedControl {
            anchors.verticalCenter: parent.verticalCenter
            model: [qsTr("Auto"), qsTr("KG"), qsTr("LB")]
            currentIndex: root.settingsVm.weightUnitMode
            onActivated: index => root.settingsVm.weightUnitMode = index
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
