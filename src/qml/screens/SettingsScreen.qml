pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var settingsVm
    required property bool compact

    property int sectionIndex: 0

    readonly property var sections: [
        qsTr("General"), qsTr("Automation"), qsTr("Services"),
        qsTr("Profiles"), qsTr("Window")
    ]

    spacing: 10

    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: Math.max(rail.implicitHeight,
                                         paneStack.children[root.sectionIndex]?.implicitHeight ?? 0)
        spacing: 12

        ColumnLayout {
            id: rail
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
            id: paneStack
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
}
