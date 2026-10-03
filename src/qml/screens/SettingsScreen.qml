pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var settingsVm

    property bool updateModeVisible: true
    property var updateVm: null
    property bool simulatorAddonsVisible: false
    property int sectionIndex: 0
    property int paneGutter: 20

    onSectionIndexChanged: paneFlick.contentY = 0

    readonly property var sections: [
        qsTr("General"), qsTr("Automation"), qsTr("Services"),
        qsTr("Profiles"), qsTr("Window"), qsTr("Advanced")
    ]

    spacing: 10

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: rail.implicitHeight
        spacing: 12

        ColumnLayout {
            id: rail
            Layout.preferredWidth: 116
            Layout.fillWidth: false
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

        Flickable {
            id: paneFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: paneStack.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            interactive: paneFlick.contentOverflows
            onContentOverflowsChanged: if (!paneFlick.contentOverflows) paneFlick.contentY = 0

            readonly property bool contentOverflows: paneFlick.contentHeight > paneFlick.height

            ScrollBar.vertical: ThemedScrollBar {
                needed: paneFlick.contentOverflows
            }

            StackLayout {
                id: paneStack
                width: paneFlick.width - root.paneGutter
                height: children[root.sectionIndex]?.implicitHeight ?? 0
                currentIndex: root.sectionIndex

                GeneralPane {
                    settingsVm: root.settingsVm
                    updateModeVisible: root.updateModeVisible
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

                AdvancedPane {
                    settingsVm: root.settingsVm
                    updateVm: root.updateVm
                    simulatorAddonsVisible: root.simulatorAddonsVisible
                }
            }
        }
    }
}
