pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qt.labs.platform as Platform

ApplicationWindow {
    id: window

    width: 620
    maximumWidth: 720
    minimumWidth: 560
    visible: !startHidden
    title: qsTr("GSX Integrator")
    color: Theme.bg

    required property var integratorVm
    required property var settingsVm
    required property var updateVm
    required property bool startHidden
    required property string trayIconSource

    readonly property bool compact: width < 620
    readonly property int shellMargin: compact ? 14 : 20

    // 0 = ops, 1 = settings, 2 = about. Header buttons toggle back to ops.
    property int screen: 0

    readonly property int fittedHeight: Math.min(
        header.height + (screens.children[window.screen]?.implicitHeight ?? 0) + 32
            + (window.screen === 1 ? settingsFooter.implicitHeight : 0),
        Screen.desktopAvailableHeight - 48)

    readonly property int lockedHeight: window.integratorVm.connected
        ? window.fittedHeight
        : Math.max(500, window.fittedHeight)

    property bool heightReady: false

    onLockedHeightChanged: Qt.callLater(window.applyLockedHeight)
    Component.onCompleted: Qt.callLater(() => {
        window.applyLockedHeight()
        window.heightReady = true
    })

    function pinHeight(h) {
        if (h > window.maximumHeight)
            window.maximumHeight = h
        window.minimumHeight = h
        window.maximumHeight = h
    }

    function applyLockedHeight() {
        const h = window.lockedHeight
        if (window.heightReady) {
            window.minimumHeight = Math.min(window.height, h)
            window.maximumHeight = Math.max(window.height, h)
            window.height = h
        } else {
            window.pinHeight(h)
            window.height = h
        }
    }

    Behavior on height {
        enabled: window.heightReady
        NumberAnimation {
            duration: 140
            easing.type: Easing.OutCubic
            onRunningChanged: if (!running) window.pinHeight(window.height)
        }
    }

    Settings {
        category: "window"
        property alias width: window.width
    }

    function restoreFromTray() {
        window.showNormal()
        window.raise()
        window.requestActivate()
    }

    function confirmQuit() {
        if (!window.visible) {
            window.restoreFromTray()
        } else {
            window.raise()
            window.requestActivate()
        }
        quitDialog.open()
    }

    function hideToTray() {
        window.hide()
        if (!window.settingsVm.trayTipShown) {
            tray.showMessage(qsTr("GSX Integrator"),
                             qsTr("Still running in the tray. Right-click the icon to open or quit."))
            window.settingsVm.trayTipShown = true
        }
    }

    onClosing: close => {
        if (window.settingsVm.closeToTray) {
            close.accepted = false
            window.hideToTray()
        } else {
            close.accepted = false
            window.confirmQuit()
        }
    }

    onVisibilityChanged: {
        if (window.visibility === Window.Minimized && window.settingsVm.minimizeToTray) {
            window.hideToTray()
        }
    }

    Dialog {
        id: quitDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        focus: true
        padding: 20

        background: Rectangle {
            radius: Theme.radius
            color: Theme.panel
            border.color: Theme.line
            border.width: 1
        }

        Overlay.modal: Rectangle {
            color: Qt.alpha(Theme.bg, 0.7)
        }

        contentItem: ColumnLayout {
            spacing: 18
            focus: true
            Keys.onReturnPressed: Qt.exit(0)
            Keys.onEnterPressed: Qt.exit(0)

            Text {
                text: qsTr("Quit GSX Integrator?")
                color: Theme.text
                font.pixelSize: 12
                font.bold: true
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
            }

            Row {
                Layout.alignment: Qt.AlignRight
                spacing: 8

                ActionButton {
                    text: qsTr("Cancel")
                    secondary: true
                    small: true
                    onClicked: quitDialog.close()
                }

                ActionButton {
                    text: qsTr("Quit")
                    tint: Theme.red
                    small: true
                    onClicked: Qt.exit(0)
                }
            }
        }
    }

    Platform.SystemTrayIcon {
        id: tray
        visible: true
        icon.source: window.trayIconSource
        tooltip: window.integratorVm.connected
                 ? qsTr("GSX Integrator") + " — " + window.integratorVm.stateText
                 : qsTr("GSX Integrator")

        menu: Platform.Menu {
            Platform.MenuItem {
                text: qsTr("Open")
                onTriggered: window.restoreFromTray()
            }
            Platform.MenuItem {
                text: qsTr("Quit")
                onTriggered: window.confirmQuit()
            }
        }

        onActivated: reason => {
            if (reason === Platform.SystemTrayIcon.Trigger
                    || reason === Platform.SystemTrayIcon.DoubleClick) {
                window.restoreFromTray()
            }
        }
    }

    Binding {
        target: Theme
        property: "dark"
        value: window.settingsVm.effectiveDark
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: header
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

                UpdatePill {
                    visible: window.updateVm.updateAvailable || window.updateVm.commbusUpdateAvailable
                    text: window.updateVm.readyToRestart ? qsTr("Restart to update")
                        : window.updateVm.updateAvailable ? "↓ v" + window.updateVm.latestVersion
                        : qsTr("↓ CommBus")
                    tip: window.updateVm.readyToRestart
                         ? qsTr("Apply the update and restart now")
                         : qsTr("Update available")
                    onClicked: {
                        if (window.updateVm.readyToRestart) {
                            window.updateVm.restartNow()
                        } else {
                            window.screen = 2
                        }
                    }
                }

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
                        updateVm: window.updateVm
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

        Rectangle {
            id: settingsFooter
            Layout.fillWidth: true
            visible: window.screen === 1
            Layout.preferredHeight: visible ? implicitHeight : 0
            implicitHeight: 60
            color: Theme.panel2

            readonly property bool showValidation: !window.settingsVm.canSave
                                                   && window.settingsVm.validationMessage.length > 0

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: Theme.line
            }

            Text {
                id: saveFeedback
                anchors.left: parent.left
                anchors.leftMargin: window.shellMargin
                anchors.right: saveButton.left
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: settingsFooter.showValidation
                      ? window.settingsVm.validationMessage
                      : window.settingsVm.saveMessage
                color: (settingsFooter.showValidation || window.settingsVm.saveError) ? Theme.red : Theme.muted
                font.pixelSize: 11
                font.capitalization: Font.AllUppercase
                elide: Text.ElideRight
                opacity: (settingsFooter.showValidation || text.length > 0) ? 1 : 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 200
                    }
                }

                Timer {
                    interval: 2500
                    running: !settingsFooter.showValidation && saveFeedback.text.length > 0
                    onTriggered: window.settingsVm.clearSaveMessage()
                }
            }

            ActionButton {
                id: saveButton
                anchors.right: parent.right
                anchors.rightMargin: window.shellMargin
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Save settings")
                enabled: window.settingsVm.canSave

                onClicked: window.settingsVm.save()
            }
        }
    }
}
