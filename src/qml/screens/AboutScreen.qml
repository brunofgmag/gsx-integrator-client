import QtQuick

Item {
    id: root

    property var updateVm
    property real viewportHeight: 0

    readonly property bool updProminent: !!updateVm
        && (updateVm.canDownload || updateVm.downloading || updateVm.readyToRestart)

    implicitHeight: Math.max(col.implicitHeight, viewportHeight - 32)

    Column {
        id: col
        y: (root.implicitHeight - implicitHeight) / 2
        width: parent.width
        spacing: 10

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/icons/app-icon_128.png"
            sourceSize: Qt.size(64, 64)
        }

        Text {
            width: parent.width
            topPadding: 8
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("GSX Integrator")
            color: Theme.text
            font.pixelSize: 22
            font.bold: true
            font.letterSpacing: 2.5
            font.capitalization: Font.AllUppercase
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Ver %1 · MSFS 2020 / 2024 · GSX Pro").arg(Qt.application.version)
            color: Theme.muted
            font.pixelSize: 10
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
        }

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(400, parent.width)
            topPadding: 14
            spacing: 10
            visible: !!root.updateVm

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.line
            }

            Text {
                width: parent.width
                topPadding: 4
                horizontalAlignment: Text.AlignHCenter
                text: root.updateVm ? root.updateVm.statusText : ""
                color: (root.updateVm && root.updateVm.hasError) ? Theme.red
                     : root.updProminent ? Theme.accent : Theme.muted
                font.pixelSize: 11
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                wrapMode: Text.WordWrap
                visible: text.length > 0
            }

            Column {
                width: parent.width
                spacing: 5
                visible: !!root.updateVm && root.updateVm.downloading

                Rectangle {
                    width: parent.width
                    height: 4
                    radius: 2
                    color: Theme.line

                    Rectangle {
                        width: parent.width * (root.updateVm ? root.updateVm.progress : 0)
                        height: parent.height
                        radius: 2
                        color: Theme.accent
                    }
                }

                Text {
                    text: qsTr("%1%").arg(Math.round((root.updateVm ? root.updateVm.progress : 0) * 100))
                    color: Theme.muted
                    font.pixelSize: 9
                    font.letterSpacing: 1
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8

                ActionButton {
                    small: true
                    secondary: true
                    visible: !!root.updateVm && root.updateVm.canCheckForUpdates
                    text: qsTr("Check for updates")
                    onClicked: root.updateVm.checkForUpdates()
                }

                ActionButton {
                    small: true
                    visible: !!root.updateVm && root.updateVm.canDownload
                    text: qsTr("Download & restart")
                    onClicked: root.updateVm.downloadAndInstall()
                }

                ActionButton {
                    small: true
                    visible: !!root.updateVm && root.updateVm.readyToRestart
                    text: qsTr("Restart now")
                    onClicked: root.updateVm.restartNow()
                }

                ActionButton {
                    small: true
                    secondary: true
                    visible: !!root.updateVm && (root.updateVm.canDownload || root.updateVm.readyToRestart)
                             && root.updateVm.releaseUrl.length > 0
                    text: qsTr("Release notes")
                    onClicked: Qt.openUrlExternally(root.updateVm.releaseUrl)
                }
            }
        }

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(400, parent.width)
            topPadding: 6
            spacing: 8
            visible: !!root.updateVm && root.updateVm.commbusUpdateAvailable

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.line
            }

            Text {
                width: parent.width
                topPadding: 4
                horizontalAlignment: Text.AlignHCenter
                text: root.updateVm
                      ? qsTr("CommBus update available — v%1").arg(root.updateVm.commbusLatestVersion)
                      : ""
                color: Theme.amber
                font.pixelSize: 11
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("The client can't update the plugin. Use the installer to update it.")
                color: Theme.muted
                font.pixelSize: 10
                font.letterSpacing: 0.8
                font.capitalization: Font.AllUppercase
                lineHeight: 1.4
                wrapMode: Text.WordWrap
            }

            ActionButton {
                anchors.horizontalCenter: parent.horizontalCenter
                small: true
                tint: Theme.amber
                visible: !!root.updateVm && root.updateVm.commbusReleaseUrl.length > 0
                text: qsTr("Get installer")
                onClicked: Qt.openUrlExternally(root.updateVm.commbusReleaseUrl)
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: root.updateVm
                      ? qsTr("Installed v%1 · latest v%2")
                            .arg(root.updateVm.commbusInstalledVersion)
                            .arg(root.updateVm.commbusLatestVersion)
                      : ""
                color: Theme.faint
                font.pixelSize: 9
                font.letterSpacing: 1
                font.capitalization: Font.AllUppercase
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(400, parent.width)
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Automates the full GSX turnaround.")
            color: Theme.muted
            font.pixelSize: 11
            font.letterSpacing: 0.5
            font.capitalization: Font.AllUppercase
            lineHeight: 1.5
            wrapMode: Text.WordWrap
        }

        Text {
            width: parent.width
            topPadding: 18
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("© 2026 · Not affiliated with FSDreamTeam or Microsoft")
            color: Theme.faint
            font.pixelSize: 10
            font.letterSpacing: 1.0
            font.capitalization: Font.AllUppercase
        }
    }
}
