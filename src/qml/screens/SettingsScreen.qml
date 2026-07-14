pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var settingsVm
    required property bool compact

    property int sectionIndex: 0

    readonly property int paneHeight: 560
    readonly property var sections: [
        qsTr("General"), qsTr("Automation"), qsTr("Services"),
        qsTr("Profiles"), qsTr("Window")
    ]

    spacing: 10

    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: root.paneHeight
        Layout.maximumHeight: root.paneHeight
        spacing: 12

        ColumnLayout {
            Layout.preferredWidth: root.compact ? 96 : 116
            Layout.alignment: Qt.AlignTop
            spacing: 2

            Repeater {
                model: root.sections

                Rectangle {
                    id: railItem

                    required property int index
                    required property string modelData
                    readonly property bool selected: root.sectionIndex === index

                    Layout.fillWidth: true
                    implicitHeight: 28
                    radius: Theme.radiusSmall
                    color: selected ? Theme.accent
                         : railHover.hovered ? Theme.panel2 : "transparent"

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: railItem.modelData
                        color: railItem.selected ? Theme.accentText : Theme.muted
                        font.pixelSize: 10
                        font.letterSpacing: 1.2
                        font.capitalization: Font.AllUppercase
                    }

                    HoverHandler {
                        id: railHover
                    }

                    TapHandler {
                        onTapped: root.sectionIndex = railItem.index
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.sectionIndex

            GeneralPane {
                settingsVm: root.settingsVm
            }

            AutomationPane {
                settingsVm: root.settingsVm
            }

            ServicesPane {
                settingsVm: root.settingsVm
            }

            ProfilesPane {
                settingsVm: root.settingsVm
            }

            WindowPane {
                settingsVm: root.settingsVm
            }
        }
    }

    Item {
        Layout.fillWidth: true
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
