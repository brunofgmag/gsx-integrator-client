pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: window

    width: 720
    height: 680
    minimumWidth: 560
    minimumHeight: 600
    visible: true
    title: qsTr("GSX Integrator")
    color: Theme.bg

    required property var integratorVm
    required property var settingsVm

    readonly property bool compact: width < 620
    readonly property int shellMargin: compact ? 14 : 20

    // 0 = ops, 1 = settings, 2 = about. Header buttons toggle back to ops.
    property int screen: 0

    Binding {
        target: Theme
        property: "dark"
        value: window.settingsVm.effectiveDark
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 46
            color: Theme.panel2

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: window.shellMargin
                anchors.rightMargin: window.shellMargin
                spacing: 8

                Row {
                    spacing: 6
                    Text {
                        text: "GSX"
                        color: Theme.text
                        font.pixelSize: 13
                        font.bold: true
                        font.letterSpacing: 2
                    }
                    Text {
                        text: "//"
                        color: Theme.accent
                        font.pixelSize: 13
                        font.bold: true
                    }
                    Text {
                        text: "INTEGRATOR"
                        color: Theme.text
                        font.pixelSize: 13
                        font.letterSpacing: 2
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                // Forces a manual light/dark, leaving "Windows" mode.
                HeaderButton {
                    text: Theme.dark ? "☀" : "☾"
                    tip: Theme.dark ? qsTr("Switch to light theme") : qsTr("Switch to dark theme")
                    onClicked: window.settingsVm.themeMode = window.settingsVm.effectiveDark ? 0 : 1
                }

                HeaderButton {
                    text: "⚙"
                    active: window.screen === 1
                    tip: qsTr("Settings")
                    onClicked: window.screen = window.screen === 1 ? 0 : 1
                }

                HeaderButton {
                    text: "i"
                    active: window.screen === 2
                    tip: qsTr("About")
                    onClicked: window.screen = window.screen === 2 ? 0 : 2
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.line
            }
        }

        Item {
            id: body
            Layout.fillWidth: true
            Layout.fillHeight: true

            readonly property bool showConnecting: window.screen === 0 && !window.integratorVm.connected

            Flickable {
                id: flick
                anchors.fill: parent
                visible: !body.showConnecting
                contentWidth: width
                contentHeight: (screens.children[screens.currentIndex]?.implicitHeight ?? 0) + 32
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                StackLayout {
                    id: screens
                    x: window.shellMargin
                    y: 16
                    width: parent.width - window.shellMargin * 2
                    currentIndex: window.screen
                    onCurrentIndexChanged: flick.contentY = 0

                    OperationsScreen {
                        integratorVm: window.integratorVm
                        settingsVm: window.settingsVm
                        compact: window.compact
                    }

                    SettingsScreen {
                        settingsVm: window.settingsVm
                        compact: window.compact
                    }

                    AboutScreen {
                        viewportHeight: body.height
                    }
                }
            }

            ConnectingPanel {
                anchors.fill: parent
                anchors.leftMargin: window.shellMargin
                anchors.rightMargin: window.shellMargin
                visible: body.showConnecting
            }
        }
    }
}
